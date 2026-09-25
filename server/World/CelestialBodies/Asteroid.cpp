#include "Asteroid.h"

#include <algorithm>
#include <cstring>
#include <random>
#include <assert.h>
#include <cmath>
#include <float.h>

#include <Utils/YamlReader.h>
#include <Utils/FloatComparator.h>

DECLARE_GLOBAL_CONTAINER_CPP(world::Asteroid);

namespace world {

Asteroid::Asteroid(uint32_t seed)
  : newton::PhysicalObject(0, 0)
  , m_randomizer(seed)
{
  utils::GlobalObject<Asteroid>::registerSelf(this);
  randomizeOrientation();
}

Asteroid::Asteroid(double radius,
                   ResourcesArray distribution,
                   uint32_t seed)
  : newton::PhysicalObject(0, radius),
    m_composition(std::move(distribution)),
    m_randomizer(seed)
{
  utils::GlobalObject<Asteroid>::registerSelf(this);
  m_composition.normalize();
  setWeight(calculateMass());
  randomizeOrientation();
}

void Asteroid::randomizeOrientation()
{
  std::uniform_real_distribution<double> distribution(0.0, 2.0 * M_PI);
  const double angle = distribution(m_randomizer);
  setOrientation(geometry::Vector(std::cos(angle), std::sin(angle)));
}

bool Asteroid::loadState(YAML::Node const& data)
{
  if (!PhysicalObject::loadState(
        data,
        PhysicalObject::LoadMask().loadPosition().loadVelocity().loadRadius()))
    return false;
  m_composition = ResourcesArray();
  utils::YamlReader reader(data);
  for (Resource::Type eType: Resource::MaterialResources) {
    reader.read(Resource::Names[eType], m_composition[eType]);
  }
  m_composition.normalize();
  setWeight(calculateMass());
  return true;
}

ResourcesArray Asteroid::yield(double amount)
{
  ResourcesArray mined;

  std::lock_guard<utils::Mutex> guard(m_mutex);
  const double mass = getWeight();

  // The request covers the whole asteroid: hand back whatever is left.
  if (amount >= mass) {
    for (Resource::Type eType: Resource::MaterialResources) {
      mined[eType] = mass * m_composition[eType];
      m_composition[eType] = 0;
    }
    // setRadius() rejects a non-positive radius.
    setWeight(0);
    setRadius(1e-3);
    return mined;
  }

  // Generating resources composition in the mined chunk
  ResourcesArray minedChunk;
  const double divider = 1.0 / m_randomizer.max();
  for (Resource::Type eType: Resource::MaterialResources) {
    const double stake = m_composition[eType];
    if (stake > DBL_EPSILON) {
      const double willOfChance = m_randomizer() * divider;
      minedChunk[eType] = 2 * stake * willOfChance;
    }
  }

  // Reduce the number of stones mined, because we are not blind to
  // mine stones!
  minedChunk[Resource::eStone] /= 2;
  minedChunk.normalize();

  double remainingMass = 0;
  for (Resource::Type eType: Resource::MaterialResources) {
    const double total = mass * m_composition[eType];
    // The chunk can ask for more of a scarce resource than the asteroid holds.
    // Take the whole remainder of that resource and no more.
    mined[eType] = std::min(amount * minedChunk[eType], total);
    const double left = std::max(0.0, total - mined[eType]);
    m_composition[eType] = left;
    remainingMass += left;
  }

  if (remainingMass < 1) {
    setWeight(0);
    setRadius(1e-3);
    return mined;
  }

  // Recalculating asteroid parameters
  m_composition.normalize();
  const double avgDensity = 1 / m_composition.calculateTotalVolume();
  const double volume     = remainingMass / avgDensity;
  const double newRadius  = pow(volume * 3.0 / (4.0 * M_PI), 1.0 / 3.0);
  setWeight(remainingMass);
  setRadius(newRadius);
  return mined;
}

double Asteroid::calculateMass() const
{
  assert(utils::AlmostEqual(m_composition.calculateTotalMass(), 1));
  const double density = 1 / m_composition.calculateTotalVolume();
  return density * 4 / 3 * M_PI * std::pow(getRadius(), 3);
}

} // namespace celestial

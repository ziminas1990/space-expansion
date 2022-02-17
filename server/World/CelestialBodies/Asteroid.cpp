#include "Asteroid.h"

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
  double mass = getWeight();
  if (mass < 1) {
    return ResourcesArray();
  }
  amount = std::min(amount, mass);

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

  for (Resource::Type eType: Resource::MaterialResources) {
    const double total = mass * m_composition[eType];
    mined[eType] = amount * minedChunk[eType];
    assert(mined[eType] <= total);
    // Recalculating composition
    m_composition[eType] = (total - mined[eType]) / mass;
  }
  
  // Recalculating asteroid parameters
  m_composition.normalize();
  mass -= amount;

  const double avgDensity = 1 / m_composition.calculateTotalVolume();
  const double volume = (mass / avgDensity);
  const double newRadius = pow(volume * 3 / (4 * M_PI), 1/3);
  setWeight(mass);
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

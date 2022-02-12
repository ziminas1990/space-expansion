#include "Asteroid.h"
#include <Utils/YamlReader.h>

#include <cstring>
#include <random>
#include <assert.h>
#include <cmath>

DECLARE_GLOBAL_CONTAINER_CPP(world::Asteroid);

namespace world {

Asteroid::Asteroid() : newton::PhysicalObject(0, 0)
{
  utils::GlobalObject<Asteroid>::registerSelf(this);
}

Asteroid::Asteroid(double radius,
                   double weight,
                   ResourcesArray distribution,
                   uint32_t seed)
  : newton::PhysicalObject(weight, radius),
    m_composition(std::move(distribution)),
    m_randomizer(seed)
{
  utils::GlobalObject<Asteroid>::registerSelf(this);
  m_composition.normalize();
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

  double weight = 2000 * 4 / 3 * M_PI * std::pow(getRadius(), 3);
  setWeight(weight);
  return true;
}

ResourcesArray Asteroid::yield(double amount)
{
  ResourcesArray mined;

  std::lock_guard<utils::Mutex> guard(m_mutex);
  amount = std::min(amount, getWeight());

  // Generating resources composition in the mined chunk
  ResourcesArray minedChunk;
  const double divider = 1.0 / m_randomizer.max();
  for (Resource::Type eType: Resource::MaterialResources) {
    const double willOfChance = m_randomizer() * divider;
    minedChunk[eType] = 2 * m_composition[eType] * willOfChance;
  }

  // Reduce the number of stones mined, because we are not blind to
  // mine stones!
  minedChunk[Resource::eStone] /= 2;
  minedChunk.normalize();

  double weight = getWeight();
  for (Resource::Type eType: Resource::MaterialResources) {
    const double total = weight * m_composition[eType];
    mined[eType] = amount * minedChunk[eType];
    assert(mined[eType] <= total);
    // Recalculating composition
    m_composition[eType] = (total - mined[eType]) / weight;
  }
  m_composition.normalize();
  changeWeight(-amount);
  return mined;
}

} // namespace celestial

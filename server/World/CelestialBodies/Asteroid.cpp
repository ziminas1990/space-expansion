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
  AsteroidsContainer::registerSelf(this);
}

Asteroid::Asteroid(double radius,
                   double weight,
                   AsteroidComposition distribution,
                   double seed)
  : newton::PhysicalObject(weight, radius),
    m_composition(std::move(distribution)),
    m_randomizer(seed)
{
  utils::GlobalContainer<Asteroid>::registerSelf(this);
}

bool Asteroid::loadState(YAML::Node const& data)
{
  if (!PhysicalObject::loadState(
        data,
        PhysicalObject::LoadMask().loadPosition().loadVelocity().loadRadius()))
    return false;
  m_composition.reset();
  utils::YamlReader reader(data);
  for (Resource::Type eType: Resource::MaterialResources) {
    reader.read(Resource::Names[eType], m_composition.m_stakes[eType]);
  }
  m_composition.normalize();

  double weight = 2000 * 4 / 3 * M_PI * std::pow(getRadius(), 3);
  setWeight(weight);
  return true;
}

ResourcesArray Asteroid::yield(double amount)
{
  ResourcesArray mined;

  m_spinlock.lock();
  amount = std::min(amount, getWeight());

  // Generating resources composition in the mined chunk
  AsteroidComposition minedChunk;
  double divider = m_randomizer.max();
  for (Resource::Type eType: Resource::MaterialResources) {
    double willOfChance = m_randomizer() / divider;
    minedChunk.m_stakes[eType] = 2 * m_composition.m_stakes[eType] * willOfChance;
  }

  // Reduce the number of stones mined, because we are not blind to
  // mine stones!
  minedChunk.m_stakes[Resource::eStone] /= 2;
  minedChunk.normalize();

  double weight = getWeight();
  for (Resource::Type eType: Resource::MaterialResources) {
    double total = weight * m_composition.m_stakes[eType];
    mined[eType] = amount * minedChunk.m_stakes[eType];
    assert(mined[eType] <= total);
    // Recalculating composition
    m_composition.m_stakes[eType] = (total - mined[eType]) / weight;
  }
  changeWeight(-amount);

  m_spinlock.unlock();
  return mined;
}

AsteroidComposition::AsteroidComposition(
    double nSilicates,
    double nMetals,
    double nIce,
    double nStones)
{
  m_stakes[Resource::Type::eIce]      = nIce;
  m_stakes[Resource::Type::eMetal]    = nMetals;
  m_stakes[Resource::Type::eSilicate] = nSilicates;
  m_stakes[Resource::Type::eStone]    = nStones;
}

void AsteroidComposition::reset()
{
  std::memset(m_stakes, 0, sizeof(m_stakes));
}

void AsteroidComposition::normalize()
{
  double total = 0;
  for (Resource::Type eType: Resource::MaterialResources) {
    total += m_stakes[eType];
  }
  if (total > 0) {
    for (Resource::Type eType: Resource::MaterialResources) {
      m_stakes[eType] /= total;
    }
  }
}

} // namespace celestial

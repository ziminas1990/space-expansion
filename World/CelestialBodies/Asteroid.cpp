#include "Asteroid.h"
#include <Utils/YamlReader.h>
#include <math.h>
#include <assert.h>

DECLARE_GLOBAL_CONTAINER_CPP(world::Asteroid);

namespace world {

Asteroid::Asteroid() : newton::PhysicalObject(0, 0)
{
  AsteroidsContainer::registerSelf(this);
}

Asteroid::Asteroid(double radius, double weight, AsteroidComposition composition)
  : newton::PhysicalObject(weight, radius),
    m_composition(std::move(composition))
{
  utils::GlobalContainer<Asteroid>::registerSelf(this);
}

bool Asteroid::loadState(YAML::Node const& data)
{
  if (!PhysicalObject::loadState(
        data,
        PhysicalObject::LoadMask().loadPosition().loadVelocity().loadRadius()))
    return false;
  utils::YamlReader reader(data);
  reader.read("silicates", m_composition.percents[Resource::Type::eSilicate])
        .read("mettals",   m_composition.percents[Resource::Type::eMettal])
        .read("ice",       m_composition.percents[Resource::Type::eIce]);
  m_composition.normalize();
  if (!reader.isOk())
    return false;

  double weight = 5000 * 4 / 3 * M_PI * std::pow(getRadius(), 3);
  setWeight(weight);
  return true;
}

double Asteroid::yield(Resource::Type eType, double amount)
{
  m_spinlock.lock();

  double total = getWeight() * m_composition.percents[eType];
  if (amount > total)
    amount = total;
  changeWeight(-amount);
  m_composition.percents[eType] = (total - amount) / getWeight();

  m_spinlock.unlock();
  return amount;
}

AsteroidComposition::AsteroidComposition(
    double utility, double nSilicates, double nMettals, double nIce)
{
  assert(utility <= 1.0 && utility >= 0.0);
  utility = std::min(utility, 1.0);
  utility = std::max(0.0,     utility);

  percents[Resource::Type::eIce]      = nIce;
  percents[Resource::Type::eMettal]   = nMettals;
  percents[Resource::Type::eSilicate] = nSilicates;
  normalize(utility);
}

void AsteroidComposition::normalize(double utility)
{
  // TODO: maybe "for" is better?
  double total = percents[Resource::Type::eIce] +
                 percents[Resource::Type::eMettal] +
                 percents[Resource::Type::eSilicate];
  for (size_t i = 0; i < world::Resource::eTotalResources; ++i) {
    percents[i] *= utility/total;
  }
}

} // namespace celestial

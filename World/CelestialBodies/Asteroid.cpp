#include "Asteroid.h"
#include <Utils/YamlReader.h>
#include <math.h>

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
  reader.read("silicates", m_composition.resources[Resources::Type::eSilicate])
        .read("mettals",   m_composition.resources[Resources::Type::eMettal])
        .read("ice",       m_composition.resources[Resources::Type::eIce]);
  m_composition.normalize();
  if (!reader.isOk())
    return false;

  double weight = 5000 * 4 / 3 * M_PI * std::pow(getRadius(), 3);
  setWeight(weight);
  return true;
}

double Asteroid::extract(Resources::Type eType, double amount)
{
  m_spinlock.lock();

  double total = getWeight() * m_composition.resources[eType];
  if (amount > total)
    amount = total;
  changeWeight(-amount);
  m_composition.resources[eType] = (total - amount) / getWeight();

  m_spinlock.unlock();
  return amount;
}

AsteroidComposition::AsteroidComposition(double nSilicates, double nMettals, double nIce)
{
  resources[Resources::Type::eIce]      = nIce;
  resources[Resources::Type::eMettal]   = nMettals;
  resources[Resources::Type::eSilicate] = nSilicates;
  normalize();
}

void AsteroidComposition::normalize()
{
  // TODO: maybe "for" is better?
  double total = resources[Resources::Type::eIce] +
                 resources[Resources::Type::eMettal] +
                 resources[Resources::Type::eSilicate];
  resources[Resources::Type::eSilicate] /= total;
  resources[Resources::Type::eMettal]   /= total;
  resources[Resources::Type::eIce]      /= total;
}

} // namespace celestial

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
  reader.read("silicates", m_composition.nSilicates)
        .read("mettals",   m_composition.nMettals)
        .read("ice",       m_composition.nIce);
  m_composition.normalize();
  if (!reader.isOk())
    return false;

  double weight = 5000 * 4 / 3 * M_PI * std::pow(getRadius(), 3);
  setWeight(weight);
  return true;
}

void AsteroidComposition::normalize()
{
  double summ = nSilicates + nMettals + nIce;
  nSilicates /= summ;
  nMettals   /= summ;
  nIce       /= summ;
}

} // namespace celestial

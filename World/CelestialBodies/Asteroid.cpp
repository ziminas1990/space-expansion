#include "Asteroid.h"
#include <Utils/YamlReader.h>

DECLARE_GLOBAL_CONTAINER_CPP(celestial::Asteroid);

namespace celestial {

Asteroid::Asteroid(double radius, double weight, AsteroidComposition composition)
  : newton::PhysicalObject(weight, radius),
    m_composition(std::move(composition))
{}

bool Asteroid::loadState(YAML::Node const& data)
{
  if (!PhysicalObject::loadState(data))
    return false;
  utils::YamlReader reader(data);
  reader.read("silicates", m_composition.nSilicates)
        .read("mettals",   m_composition.nMettals)
        .read("ice",       m_composition.nIce);
  return reader.isOk();
}

} // namespace celestial

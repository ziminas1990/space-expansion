#pragma once

#include <Utils/GlobalContainer.h>
#include <Newton/PhysicalObject.h>

#include <Utils/YamlForwardDeclarations.h>

namespace celestial {

struct AsteroidComposition
{
  AsteroidComposition(uint32_t nSilicates, uint32_t nMettals, uint32_t nIce)
    : nSilicates(nSilicates), nMettals(nMettals), nIce(nIce)
  {}

  uint32_t nSilicates;
  uint32_t nMettals;
  uint32_t nIce;
};

class Asteroid :
    public newton::PhysicalObject,
    public utils::GlobalContainer<Asteroid>
{
public:
  Asteroid(double radius, double weight, AsteroidComposition composition);

  bool loadState(YAML::Node const& data);

  AsteroidComposition const& getComposition() const { return m_composition; }

private:
  AsteroidComposition m_composition;

};

} // namespace celestial

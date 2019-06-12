#pragma once

#include <memory>
#include <Utils/GlobalContainer.h>
#include <Newton/PhysicalObject.h>

#include <Utils/YamlForwardDeclarations.h>

namespace world {

struct AsteroidComposition
{
  AsteroidComposition(double nSilicates, double nMettals, double nIce)
    : nSilicates(nSilicates), nMettals(nMettals), nIce(nIce)
  {
    normalize();
  }
  AsteroidComposition() : AsteroidComposition(1, 1, 1)
  {}

  void normalize();

  double nSilicates;
  double nMettals;
  double nIce;
};

class Asteroid :
    public newton::PhysicalObject,
    public utils::GlobalContainer<Asteroid>
{
public:
  Asteroid();
  Asteroid(double radius, double weight, AsteroidComposition composition);

  bool loadState(YAML::Node const& data);

  AsteroidComposition const& getComposition() const { return m_composition; }

  uint32_t getAsteroidId() const {
    return utils::GlobalContainer<Asteroid>::getInstanceId();
  }

private:
  AsteroidComposition m_composition;

};

using AsteroidUptr = std::unique_ptr<Asteroid>;

} // namespace celestial

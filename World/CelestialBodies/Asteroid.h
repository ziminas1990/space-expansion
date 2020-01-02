#pragma once

#include <memory>
#include <Utils/GlobalContainer.h>
#include <Newton/PhysicalObject.h>
#include <Utils/Spinlock.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/Resources.h>

namespace world {

struct AsteroidComposition
{
  AsteroidComposition(double nSilicates, double nMettals, double nIce);
  AsteroidComposition() : AsteroidComposition(1, 1, 1)
  {}

  double silicates_percent() const { return resources[Resources::Type::eSilicate]; }
  double mettals_percent()   const { return resources[Resources::Type::eMettal]; }
  double ice_percent()       const { return resources[Resources::Type::eIce]; }

  void normalize();

  double resources[Resources::Type::eTotalResources];
};

class Asteroid;
using AsteroidsContainer = utils::GlobalContainer<Asteroid>;

class Asteroid : public newton::PhysicalObject, public AsteroidsContainer
{
public:
  Asteroid();
  Asteroid(double radius, double weight, AsteroidComposition composition);

  bool loadState(YAML::Node const& data);

  AsteroidComposition const& getComposition() const { return m_composition; }

  uint32_t getAsteroidId() const {
    return utils::GlobalContainer<Asteroid>::getInstanceId();
  }

  double extract(Resources::Type eType, double amount);

private:
  AsteroidComposition m_composition;

  utils::Spinlock m_spinlock;
};

using AsteroidUptr = std::unique_ptr<Asteroid>;

} // namespace world

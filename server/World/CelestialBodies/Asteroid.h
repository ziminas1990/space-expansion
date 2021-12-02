#pragma once

#include <memory>
#include <random>

#include <Utils/GlobalContainer.h>
#include <Newton/PhysicalObject.h>
#include <Utils/Spinlock.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/Resources.h>
#include <World/ObjectTypes.h>

namespace world {

struct AsteroidComposition
{
  AsteroidComposition(double nSilicates,
                      double nMetals,
                      double nIce,
                      double nStones);
  AsteroidComposition()
  {
    reset();
  }

  void   reset();
  double silicates_percent() const { return m_stakes[Resource::Type::eSilicate]; }
  double metals_percent()    const { return m_stakes[Resource::Type::eMetal]; }
  double ice_percent()       const { return m_stakes[Resource::Type::eIce]; }

  void normalize();

  double m_stakes[Resource::Type::eTotalResources];
};

class Asteroid;
using AsteroidsContainer = utils::GlobalContainer<Asteroid>;

class Asteroid :
    public newton::PhysicalObject,
    public utils::GlobalObject<Asteroid>
{
public:
  Asteroid();
  Asteroid(double radius,
           double weight,
           AsteroidComposition distribution,
           double seed);

  bool loadState(YAML::Node const& data);

  AsteroidComposition const& getComposition() const { return m_composition; }

  uint32_t getAsteroidId() const {
    return utils::GlobalObject<Asteroid>::getInstanceId();
  }

  ResourcesArray yield(double amount);

  world::ObjectType getType() const override {
    return world::ObjectType::eAsteroid;
  }

private:
  AsteroidComposition        m_composition;
  std::default_random_engine m_randomizer;

  utils::Spinlock m_spinlock;
};

using AsteroidUptr = std::unique_ptr<Asteroid>;

} // namespace world

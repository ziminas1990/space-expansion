#pragma once

#include <memory>
#include <random>

#include <Utils/GlobalContainer.h>
#include <Newton/PhysicalObject.h>
#include <Utils/Mutex.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/Resources.h>
#include <World/ObjectTypes.h>

namespace world {

class Asteroid;
using AsteroidsContainer = utils::GlobalContainer<Asteroid>;

class Asteroid :
    public newton::PhysicalObject,
    public utils::GlobalObject<Asteroid>
{
public:
  using Ptr  = std::shared_ptr<Asteroid>;
  using Uptr = std::unique_ptr<Asteroid>;

public:
  Asteroid(uint32_t seed);
  Asteroid(double radius,
           double weight,
           ResourcesArray distribution,
           uint32_t seed);

  bool loadState(YAML::Node const& data);

  ResourcesArray const& getComposition() const { return m_composition; }

  uint32_t getAsteroidId() const {
    return utils::GlobalObject<Asteroid>::getInstanceId();
  }

  ResourcesArray yield(double amount);

  world::ObjectType getType() const override {
    return world::ObjectType::eAsteroid;
  }

private:
  ResourcesArray             m_composition;
  std::default_random_engine m_randomizer;

  utils::Mutex m_mutex;
};

using AsteroidUptr = Asteroid::Uptr;

} // namespace world

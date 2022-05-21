#include "World.h"

#include "CelestialBodies/AsteroidGenerator.h"
#include <Utils/YamlReader.h>
#include <yaml-cpp/yaml.h>

namespace world {

World::World(unsigned long seed)
  : m_randomizer(seed)
{}

bool World::loadState(YAML::Node const& data)
{
  YAML::Node const& asteroidsData = data["Asteroids"];
  if (asteroidsData.IsDefined()) {
    m_asteroids.reserve(asteroidsData.size());
    for (YAML::Node const& asteroidData : asteroidsData) {
      AsteroidUptr pAsteroid = std::make_unique<Asteroid>(m_randomizer.yield());
      if (!pAsteroid->loadState(asteroidData)) {
        assert(false);
        return false;
      }
      m_asteroids.push_back(std::move(pAsteroid));
    }
  }

  YAML::Node const& cloudsData = data["AsteroidsClouds"];
  if (asteroidsData.IsDefined()) {
    for (YAML::Node const& cloudData : cloudsData) {
      AsteroidGenerator generator;
      if (!generator.loadState(cloudData)) {
        assert(false);
        return false;
      }
      uint32_t nAsteroidsInCloud = 0;
      if (!utils::YamlReader(cloudData).read("total", nAsteroidsInCloud)) {
        assert(false);
        return false;
      }
      generator.generate(nAsteroidsInCloud, m_asteroids);
    }
  }
  return true;
}

uint32_t World::spawnAsteroid(const ResourcesArray& distribution,
                              double radius,
                              const geometry::Point &position,
                              const geometry::Vector &velocity)
{
  AsteroidUptr pAsteroid = std::make_unique<Asteroid>(
        radius, distribution, m_randomizer.yield());
  pAsteroid->moveTo(position);
  pAsteroid->setVelocity(velocity);
  const uint32_t asteroid_id = pAsteroid->getAsteroidId();
  m_asteroids.emplace_back(std::move(pAsteroid));
  return asteroid_id;
}


} // namespace world

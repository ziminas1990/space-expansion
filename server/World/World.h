#pragma once

#include <vector>
#include "CelestialBodies/Asteroid.h"
#include <Utils/YamlForwardDeclarations.h>
#include <Utils/RandomSequence.h>

namespace world {

class World
{
public:
  World(unsigned long seed);

  bool loadState(YAML::Node const& data);

  uint32_t spawnAsteroid(const ResourcesArray&   distribution,
                         double                  radius,
                         const geometry::Point&  position,
                         const geometry::Vector& velocity);

private:
  utils::RandomSequence     m_randomizer;
  std::vector<AsteroidUptr> m_asteroids;
};

} // namespace world

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

private:
  utils::RandomSequence     m_randomizer;
  std::vector<AsteroidUptr> m_asteroids;
};

} // namespace world

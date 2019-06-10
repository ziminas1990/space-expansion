#pragma once

#include <vector>
#include "CelestialBodies/Asteroid.h"
#include <Utils/YamlForwardDeclarations.h>

namespace world {

class World
{
public:

  bool loadState(YAML::Node const& data);

private:
  std::vector<AsteroidUptr> m_asteroids;
};

} // namespace world

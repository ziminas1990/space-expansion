#pragma once

#include <stdint.h>
#include <Geometry/Point.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/CelestialBodies/Asteroid.h>

namespace world {

class AsteroidGenerator
{
public:
  AsteroidGenerator() : m_pattern(0), m_areaRadiusKm(0) {}
  AsteroidGenerator(uint32_t pattern, geometry::Point center, double areaRadiusKm)
    : m_pattern(pattern), m_center(std::move(center)), m_areaRadiusKm(areaRadiusKm)
  {}

  bool loadState(YAML::Node const& data);

  void generate(uint32_t nCount, std::vector<AsteroidUptr> &out);

private:
  uint32_t        m_pattern;
  geometry::Point m_center;
  double          m_areaRadiusKm;
};

} // namespace celestial

#include "AsteroidGenerator.h"

#include <cmath>

#include <Utils/YamlReader.h>
#include <Utils/Randomizer.h>

bool world::AsteroidGenerator::loadState(YAML::Node const& data)
{
  return utils::YamlReader(data)
      .read("pattern",        m_pattern)
      .read("center",         m_center)
      .read("area_radius_km", m_areaRadiusKm);
}

void world::AsteroidGenerator::generate(uint32_t nCount, std::vector<AsteroidUptr> &out)
{
  out.reserve(out.size() + nCount);
  utils::Randomizer::setPattern(m_pattern);
  while (nCount--)
  {
    geometry::Point  position;
    double           radius;
    utils::Randomizer::yeild(position, m_center, m_areaRadiusKm * 1000);
    radius = utils::Randomizer::yeild(5.0, 15.0);

    double weight = 5000 * 4 / 3 * M_PI * std::pow(radius, 3);
    AsteroidComposition composition(
      utils::Randomizer::yeild(0.05, 0.3),
      utils::Randomizer::yeild(0.0,  1.0),
      utils::Randomizer::yeild(0.0,  1.0),
      utils::Randomizer::yeild(0.0,  1.0));

    AsteroidUptr pAsteroid = std::make_unique<Asteroid>(radius, weight, composition);
    pAsteroid->moveTo(position);
    out.emplace_back(std::move(pAsteroid));
  }
}

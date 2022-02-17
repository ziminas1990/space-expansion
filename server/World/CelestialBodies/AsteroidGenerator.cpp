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
    utils::Randomizer::yield(position, m_center, m_areaRadiusKm * 1000);
    radius = utils::Randomizer::yield(5.0, 15.0);

    ResourcesArray composition = ResourcesArray()
        .silicates(utils::Randomizer::yield(0.0, 1.0))
        .metals(utils::Randomizer::yield(0.0, 1.0))
        .ice(utils::Randomizer::yield(0.0, 1.0))
        .stones(utils::Randomizer::yield(5, 20));

    AsteroidUptr pAsteroid = std::make_unique<Asteroid>(
          radius, composition, std::rand());
    pAsteroid->moveTo(position);
    out.emplace_back(std::move(pAsteroid));
  }
}

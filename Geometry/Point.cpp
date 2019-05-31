#include "Point.h"

#include <Utils/YamlReader.h>

namespace geometry
{

bool Point::load(YAML::Node const& node)
{
  return utils::YamlReader(node).read("x", x).read("y", y);
}

} // namespace geomtery

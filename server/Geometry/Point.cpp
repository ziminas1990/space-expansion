#include "Point.h"

#include <Utils/YamlReader.h>
#include <Utils/YamlDumper.h>

namespace geometry
{

bool Point::load(YAML::Node const& node)
{
  return utils::YamlReader(node).read("x", x).read("y", y);
}

void Point::dump(YAML::Node& out) const
{
  utils::YamlDumper(out).add("x", x).add("y", y);
}

} // namespace geomtery

#include "Vector.h"

#include <Utils/YamlReader.h>

namespace geometry
{

bool Vector::load(YAML::Node const& node)
{
  return utils::YamlReader(node).read("x", x).read("y", y);
}

} // namespace geomtery

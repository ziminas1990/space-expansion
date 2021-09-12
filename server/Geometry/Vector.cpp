#include "Vector.h"

#include <iostream>
#include <Utils/YamlReader.h>
#include <Utils/YamlDumper.h>

namespace geometry
{

bool Vector::load(YAML::Node const& node)
{
  return utils::YamlReader(node).read("x", x).read("y", y);
}

void Vector::dump(YAML::Node& out) const
{
  utils::YamlDumper(out).add("x", x).add("y", y);
}

std::ostream& operator<<(std::ostream& out, Vector const& vector)
{
  return out << "{" << vector.getX() << ", " << vector.getY() << "}";
}

} // namespace geomtery

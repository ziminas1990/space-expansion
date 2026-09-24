#include "Vector.h"

#include <iostream>
#include <Utils/YamlReader.h>
#include <Utils/YamlDumper.h>

namespace geometry
{

namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

bool Vector::load(YAML::Node const& node)
{
  return utils::YamlReader(node).read("x", x).read("y", y);
}

void Vector::dump(YAML::Node& out) const
{
  utils::YamlDumper(out).add("x", x).add("y", y);
}

double Vector::shortestTurn(Vector const& to) const
{
  const double scale = getLength() * to.getLength();
  if (!(scale > 0.0)) {
    return 0.0;
  }
  const double cross = (x * to.y - y * to.x) / scale;
  const double dot   = (x * to.x + y * to.y) / scale;
  const double angle = std::atan2(cross, dot);
  if (std::abs(std::abs(angle) - kPi) < 1e-6) {
    return kPi;
  }
  return angle;
}

std::ostream& operator<<(std::ostream& out, Vector const& vector)
{
  return out << "{" << vector.getX() << ", " << vector.getY() << "}";
}

} // namespace geomtery

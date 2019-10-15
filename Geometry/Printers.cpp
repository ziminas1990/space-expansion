#include "Printers.h"

namespace geometry {

std::ostream& operator<<(std::ostream& out, Point const& point)
{
  return out << "(" << point.x << ", " << point.y << ")";
}

std::ostream& operator<<(std::ostream& out, Vector const& vector)
{
  return out << "{" << vector.getX() << ", " << vector.getY() << "}";
}

} // namespace geometry

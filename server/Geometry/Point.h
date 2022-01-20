#pragma once

#include <iosfwd>
#include <float.h>
#include <math.h>
#include "Vector.h"
#include <Utils/YamlForwardDeclarations.h>
#include <Utils/FloatComparator.h>


namespace geometry
{

struct Point
{
  Point() : x(0.0), y(0.0) {}
  Point(double x, double y) : x(x), y(y) {}

  bool operator==(Point const& other) const {
    return utils::AlmostEqual(x, other.x)
        && utils::AlmostEqual(y, other.y);
  }

  double distanceSqr(Point const& other) const
  {
    const double dx = x - other.x;
    const double dy = y - other.y;
    return dx * dx + dy * dy;
  }

  double distance(Point const& other) const
  {
    return sqrt(distanceSqr(other));
  }

  Vector vectorTo(Point const& other) const
  {
    return Vector(other.x - x, other.y - y);
  }

  bool almostEqual(Point const& other, double epsilon) const
  {
    return distance(other) < epsilon;
  }

  Point& translate(Vector const& v) {
    x += v.getX();
    y += v.getY();
    return *this;
  }

  Point translated(Vector const& v) const {
    return Point(x, y).translate(v);
  }

  Point operator+(Vector const& v) const { return Point(x + v.getX(), y + v.getY()); }

  Point& operator+=(Vector const& v) {
    x += v.getX();
    y += v.getY();
    return *this;
  }

  bool load(YAML::Node const& node);
  void dump(YAML::Node& out) const;

  double x;
  double y;
};

std::ostream& operator<<(std::ostream& out, const Point& point);

} // namespace geomtery

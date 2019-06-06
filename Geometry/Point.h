#pragma once

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

  double distance(Point const& other) const
  {
    double dx = x - other.x;
    double dy = y - other.y;
    return sqrt(dx * dx + dy * dy);
  }

  bool almostEqual(Point const& other, double epsilon) const
  {
    return distance(other) < epsilon;
  }

  void translate(Vector const& v) {
    x += v.getX();
    y += v.getY();
  }

  Point operator+(Vector const& v) const { return Point(x + v.getX(), y + v.getY()); }

  bool load(YAML::Node const& node);

  double x;
  double y;
};

} // namespace geomtery

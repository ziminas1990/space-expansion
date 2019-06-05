#pragma once

#include <float.h>
#include <math.h>
#include <Utils/YamlForwardDeclarations.h>
#include <Utils/FloatComparator.h>

namespace geometry
{

struct Point
{
  Point() : x(0), y(0) {}
  Point(double x, double y) : x(x), y(y) {}

  bool operator==(Point const& other) const {
    return utils::AlmostEqual(x, other.x)
        && utils::AlmostEqual(y, other.y);
  }

  bool load(YAML::Node const& node);

  double x;
  double y;
};

} // namespace geomtery

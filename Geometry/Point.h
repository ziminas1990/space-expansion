#pragma once

#include <Utils/YamlForwardDeclarations.h>

namespace geometry
{

struct Point
{
  Point() : x(0), y(0) {}
  Point(double x, double y) : x(x), y(y) {}

  bool load(YAML::Node const& node);

  double x;
  double y;
};

} // namespace geomtery

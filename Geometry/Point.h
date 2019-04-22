#pragma once

namespace geometry
{

struct Point
{
  Point() : x(0), y(0) {}
  Point(double x, double y) : x(x), y(y) {}

  double x;
  double y;
};

} // namespace geomtery

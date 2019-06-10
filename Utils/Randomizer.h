#pragma once

#include <cstdlib>

namespace geometry {
  class Point;
} // namespace geometry

namespace utils {

class Randomizer
{
public:
  static void setPattern(unsigned nPattern);

  static void yeild(geometry::Point& point,
                    geometry::Point const& center,
                    double radius);

  template<typename Type>
  static Type yeild(Type bottom, Type top)
  {
    return bottom + (top - bottom) * (std::rand() / double(RAND_MAX));
  }

};

} // namespace utils

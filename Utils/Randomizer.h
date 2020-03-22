#pragma once

#include <cstdlib>

namespace geometry {
  struct Point;
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

  static void splitToParts(double parts[], size_t nTotal)
  {
    double sum = 0;
    for (size_t i = 0; i < nTotal; ++i) {
      parts[i] = std::rand() / double(RAND_MAX);
      sum += parts[i];
    }

    for (size_t i = 0; i < nTotal; ++i) {
      parts[i] /= sum;
    }
  }

};

} // namespace utils

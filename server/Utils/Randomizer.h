#pragma once

#include <cstdlib>

namespace geometry {
  struct Point;
  struct Vector;
  struct Rectangle;
} // namespace geometry

namespace utils {

class Randomizer
{
public:
  static void setPattern(unsigned nPattern);

  static void yield(geometry::Point& point,
                    geometry::Point const& center,
                    double radius);

  static void yield(geometry::Vector& vec, double radius);

  template<typename Type>
  static Type yield(Type bottom, Type top)
  {
    return bottom + (top - bottom) * (std::rand() / double(RAND_MAX));
  }

  static void yield(geometry::Point& point,
                    const geometry::Rectangle& parent);
    // Spawn a point inside the specified 'parent'

  static void yield(geometry::Rectangle& rect,
                    const geometry::Rectangle& parent);
    // Spawn a rect inside the specified 'parent'.

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

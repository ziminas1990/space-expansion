#pragma once

#include <math.h>
#include <float.h>

namespace utils {

inline bool AlmostEqual(double a, double b)
{
  // Nice article about that:
  // https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
  return fabs(a - b) <= fabs(a) * DBL_EPSILON;
}

} // namespace utils

#include "Randomizer.h"
#include <Geometry/Point.h>

#include <cmath>

namespace utils {

void Randomizer::setPattern(unsigned nPattern) { std::srand(nPattern); }

void Randomizer::yeild(geometry::Point& point,
                       geometry::Point const& center,
                       double radius)
{
  double alfa = std::rand() * M_PI / RAND_MAX;
  double r    = std::rand() * radius / RAND_MAX;
  point.x = center.x + r * cos(alfa);
  point.y = center.y + r * sin(alfa);
}



} // namespace utils

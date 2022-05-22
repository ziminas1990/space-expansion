#include "Randomizer.h"
#include <Geometry/Point.h>
#include <Geometry/Rectangle.h>

#include <cmath>

namespace utils {

void Randomizer::setPattern(unsigned nPattern) { std::srand(nPattern); }

void Randomizer::yield(geometry::Point &point,
                       const geometry::Point &center,
                       double minRadius,
                       double maxRadius)
{
  geometry::Vector offset;
  yield(offset, minRadius*minRadius, maxRadius*maxRadius);
  offset.setLength(sqrt(offset.getLength()));
  point = center.translated(offset);
}

void Randomizer::yield(geometry::Point& point,
                       geometry::Point const& center,
                       double maxRadius)
{
  return yield(point, center, 0, maxRadius);
}

void Randomizer::yield(geometry::Vector& vec, double maxRadius)
{
  return yield(vec, 0, maxRadius);
}

void Randomizer::yield(geometry::Vector &vec,
                       double minRadius,
                       double maxRadius)
{
  double alfa = std::rand() * 2 * M_PI / RAND_MAX;
  double r    = yield<double>(minRadius, maxRadius);
  vec.setPosition(r * cos(alfa), r * sin(alfa));
}

void Randomizer::yield(geometry::Point &point,
                       const geometry::Rectangle &parent)
{
  point = geometry::Point(
        Randomizer::yield(parent.left(), parent.right()),
        Randomizer::yield(parent.bottom(), parent.top()));
}

void Randomizer::yield(geometry::Rectangle &rect,
                       const geometry::Rectangle &parent)
{
  geometry::Point topLeft;
  Randomizer::yield(topLeft, parent);

  geometry::Point rightBottom;
  Randomizer::yield(
        rightBottom,
        geometry::Rectangle(topLeft, parent.m_position[1]));

  rect = geometry::Rectangle(topLeft, rightBottom);
}



} // namespace utils

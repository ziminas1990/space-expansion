#pragma once

#include <assert.h>

#include "Point.h"

namespace geometry {

struct Rectangle
{
  Rectangle() = default;

  Rectangle(Point const& center, double width, double height)
  {
    width  /= 2;
    height /= 2;
    m_position[0].x = center.x - width;
    m_position[0].y = center.y + height;
    m_position[1].x = center.x + width;
    m_position[1].y = center.y - height;
  }

  Rectangle(Point const& leftUp, Point const& rightDown)
  {
    assert(leftUp.x <= rightDown.x);
    assert(leftUp.y >= rightDown.y);

    m_position[0] = leftUp;
    m_position[1] = rightDown;
  }

  bool isCoveredByCicle(Point const& center, double r) const
  {
    // Check if at least one point of the circle with the specified 'center'
    // and 'r' is belong to the rectangle
    return center.x + r > m_position[0].x
        && center.x - r < m_position[1].x
        && center.y - r < m_position[0].y
        && center.y + r > m_position[1].y;
  }

  Point m_position[2];
};

} // namespace geomtery

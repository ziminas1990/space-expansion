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

  double left()   const { return m_position[0].x; }
  double right()  const { return m_position[1].x; }
  double top()    const { return m_position[0].y; }
  double bottom() const { return m_position[1].y; }
  double width()  const { return right() - left(); }
  double height() const { return top() - bottom(); }

  bool isCoveredByCircle(Point const& center, double r) const
  {
    // Check if at least one point of the circle with the specified 'center'
    // and 'r' is belong to the rectangle
    return center.x + r > m_position[0].x
        && center.x - r < m_position[1].x
        && center.y - r < m_position[0].y
        && center.y + r > m_position[1].y;
  }

  Point center() const {
    return Point((m_position[0].x + m_position[1].x) / 2,
                 (m_position[0].y + m_position[1].y) / 2);
  }

  template<typename NumericType>
  Rectangle& extend(NumericType k) {
    const Point& c = center();
    for (Point& point: m_position) {
      point.x = c.x + (point.x - c.x) * k;
      point.y = c.y + (point.y - c.y) * k;
    }
    return *this;
  }

  template<typename NumericType>
  Rectangle extend(NumericType k) const {
    Rectangle r = *this;
    return r.extend(k);
  }

  Point m_position[2];
};

} // namespace geomtery

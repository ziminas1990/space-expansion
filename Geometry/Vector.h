#pragma once

#include "Point.h"
#include <math.h>

namespace geometry {

class Vector
{
public:
  Vector() = default;
  Vector(Point position) : m_position(position) {}

  Point const& getPosition() const { return m_position; }

  void setPosition(double x, double y)
  {
    m_position.x       = x;
    m_position.y       = y;
    m_length.lIsActual = false;
  }

  double getLength() const
  {
    if (!m_length.lIsActual) {
      m_length.nValue = std::sqrt(m_position.x * m_position.x +
                                  m_position.y * m_position.y);
      m_length.lIsActual = true;
    }
    return m_length.nValue;
  }

private:
  Point m_position;

  mutable struct {
    bool   lIsActual = false;
    double nValue    = 0;
  } m_length;
};

} // namespace geomtery

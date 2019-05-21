#pragma once

#include "Point.h"
#include <math.h>

namespace geometry {

class Vector
{
public:
  Vector() = default;
  Vector(Point position) : m_position(position) {}
  Vector(Vector const& other, double k = 1)
    : m_position(other.m_position.x * k, other.m_position.y * k)
  {}

  Point const& getPosition() const { return m_position; }

  void setPosition(double x, double y)
  {
    m_position.x       = x;
    m_position.y       = y;
    m_length.lIsActual = false;
  }

  Vector& add(Vector const& other, double k) {
    m_position.x += other.m_position.x * k;
    m_position.y += other.m_position.y * k;
    m_length.lIsActual = false;
    return *this;
  }

  Vector& operator*= (double k) {
    m_position.x *= k;
    m_position.y *= k;
    m_length.lIsActual = false;
    return *this;
  }

  Vector& operator+= (Vector const& other) {
    m_position.x += other.m_position.x;
    m_position.y += other.m_position.y;
    m_length.lIsActual = false;
    return *this;
  }

  double getSqrLength() const
  {
    if (!m_length.lIsActual)
      calculateLength();
    return m_length.nSqrValue;
  }

  double getLength() const
  {
    if (!m_length.lIsActual)
      calculateLength();
    return m_length.nValue;
  }

private:
  // Yeah, it is "const"
  void calculateLength() const {
    m_length.nSqrValue = m_position.x * m_position.x + m_position.y * m_position.y;
    m_length.nValue    = std::sqrt(m_length.nSqrValue);
    m_length.lIsActual = true;
  }

private:
  Point m_position;

  mutable struct {
    bool   lIsActual = false;
    double nSqrValue = 0;
    double nValue    = 0;
  } m_length;
};

} // namespace geomtery

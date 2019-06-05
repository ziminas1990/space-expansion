#pragma once

#include "Point.h"
#include <math.h>

#include <Utils/YamlForwardDeclarations.h>

namespace geometry {

class Vector
{
public:
  Vector() = default;
  Vector(Point position) : m_position(position) {}
  Vector(Vector const& other, double k = 1)
    : m_position(other.m_position.x * k, other.m_position.y * k)
  {}

  bool operator==(Vector const& other) const {
    return m_position == other.m_position;
  }

  bool load(YAML::Node const& node) {
    m_length.lIsActual = false;
    return m_position.load(node);
  }

  Point const& getPosition() const { return m_position; }
  double getSqrLength() const
  {
    calculateLength();
    return m_length.nSqrValue;
  }

  double getLength() const
  {
    calculateLength();
    return m_length.nValue;
  }

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

  void normalize()
  {
    calculateLength();
    m_position.x      /= m_length.nValue;
    m_position.y      /= m_length.nValue;
    m_length.nValue    = 1;
    m_length.nSqrValue = 1;
  }

  void setLength(double newLength)
  {
    normalize();
    m_position.x      *= newLength;
    m_position.y      *= newLength;
    m_length.nValue    = newLength;
    m_length.nSqrValue = newLength * newLength;
  }

  void toZero()
  {
    m_position.x      *= 0;
    m_position.y      *= 0;
    m_length.nValue    = 0;
    m_length.nSqrValue = 0;
    m_length.lIsActual = true;
  }

private:
  // Yeah, it is "const"
  void calculateLength() const {
    if (!m_length.lIsActual) {
      m_length.nSqrValue = m_position.x * m_position.x + m_position.y * m_position.y;
      m_length.nValue    = std::sqrt(m_length.nSqrValue);
      m_length.lIsActual = true;
    }
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

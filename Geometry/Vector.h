#pragma once

#include <math.h>

#include <Utils/FloatComparator.h>
#include <Utils/YamlForwardDeclarations.h>

namespace geometry {

class Vector
{
public:
  Vector() : x(0), y(0) {}
  Vector(double x, double y) : x(x), y(y) {}
  Vector(Vector const& other, double k = 1)
    : x(other.x * k), y(other.y * k)
  {}

  bool operator==(Vector const& other) const {
    return utils::AlmostEqual(x, other.x)
        && utils::AlmostEqual(y, other.y);
  }

  bool almostEqual(Vector const& other, double epsilon) const
  {
    double dx = x - other.x;
    double dy = y - other.y;
    return dx * dx + dy * dy < epsilon * epsilon;
  }

  bool load(YAML::Node const& node);

  double getX() const { return x; }
  double getY() const { return y; }

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
    this->x = x;
    this->y = y;
    m_length.lIsActual = false;
  }

  Vector& add(Vector const& other, double k) {
    x += other.x * k;
    y += other.y * k;
    m_length.lIsActual = false;
    return *this;
  }

  Vector& operator+= (Vector const& other) {
    x += other.x;
    y += other.y;
    m_length.lIsActual = false;
    return *this;
  }

  Vector operator+ (Vector const& other) const {
    return Vector(x + other.x, y + other.y);
  }
  Vector operator- (Vector const& other) const {
    return Vector(x - other.x, y - other.y);
  }

  Vector& operator*= (double k) {
    x *= k;
    y *= k;
    if (m_length.lIsActual) {
      m_length.nValue    *= k;
      m_length.nSqrValue *= k * k;
    }
    return *this;
  }

  Vector operator*(double k) const {
    return m_length.lIsActual ? Vector(x * k, y * k, m_length.nValue * k)
                              : Vector(x * k, y * k);
  }

  double operator*(Vector const& other) {
    return (x * other.x + y * other.y) / (getLength() * other.getLength());
  }

  Vector operator/(double k) const { return operator*(1/k); }

  void normalize()
  {
    calculateLength();
    x /= m_length.nValue;
    y /= m_length.nValue;
    m_length.nValue    = 1;
    m_length.nSqrValue = 1;
  }

  void setLength(double newLength)
  {
    normalize();
    x *= newLength;
    y *= newLength;
    m_length.nValue    = newLength;
    m_length.nSqrValue = newLength * newLength;
  }

  void toZero()
  {
    x = 0;
    y = 0;
    m_length.nValue    = 0;
    m_length.nSqrValue = 0;
    m_length.lIsActual = true;
  }

private:
  Vector(double x, double y, double length)
    : x(x), y(y)
  {
    m_length.nValue    = length;
    m_length.nSqrValue = length * length;
    m_length.lIsActual = true;
  }

  // Yeah, it is "const"
  void calculateLength() const {
    if (!m_length.lIsActual) {
      m_length.nSqrValue = x * x + y * y;
      m_length.nValue    = std::sqrt(m_length.nSqrValue);
      m_length.lIsActual = true;
    }
  }

private:
  double x;
  double y;

  mutable struct {
    bool   lIsActual = false;
    double nSqrValue = 0;
    double nValue    = 0;
  } m_length;
};

} // namespace geomtery

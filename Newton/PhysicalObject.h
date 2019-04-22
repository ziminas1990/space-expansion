#pragma once

#include <Geometry/Point.h>
#include <Geometry/Vector.h>

namespace newton {

class PhysicalObject
{
public:

  geometry::Point const&  getPosition() const { return m_Position; }
  geometry::Vector const& getVelocity() const { return m_Velocity; }

private:
  geometry::Point  m_Position;
  // NOTE: velocity vector stores a velocity as meters pre MICRO seconds!
  geometry::Vector m_Velocity;
};

} // namespace newton

#include "PhysicalObject.h"

#include <algorithm>
#include <cmath>
#include <mutex>  // for std::lock_guard

#include <Utils/YamlReader.h>

DECLARE_GLOBAL_CONTAINER_CPP(newton::PhysicalObject);

namespace newton {

PhysicalObject::PhysicalObject(double weight, double radius)
  : m_radius(radius), m_orientation(1.0, 0.0), m_pCell(nullptr)
{
  GlobalObject<PhysicalObject>::registerSelf(this);
  setWeight(weight);
  m_forces.reserve(4);
}

bool PhysicalObject::loadState(YAML::Node const& data, LoadMask mask)
{
  utils::YamlReader reader(data);
  if (mask.nValue & LoadMask::eLoadPosition)
    reader.read("position", m_position);
  if (mask.nValue & LoadMask::eLoadVelocity)
    reader.read("velocity", m_velocity);
  if (mask.nValue & LoadMask::eLoadWeight)
    reader.read("weight", m_weight);
  if (mask.nValue & LoadMask::eLoadRadius)
    reader.read("radius", m_radius);
  if (mask.nValue & LoadMask::eLoadOrientation)
    reader.read("orientation", m_orientation);
  if (!reader.isOk())
    return false;

  if (mask.nValue & LoadMask::eLoadOrientation) {
    if (!(m_orientation.getLength() > 0.0))
      return false;
    m_orientation.normalize();
  }

  return true;
}

void PhysicalObject::setOrientation(geometry::Vector orientation)
{
  if (!(orientation.getLength() > 0.0)) {
    return;
  }
  orientation.normalize();
  const double cosine = m_orientation.getX() * orientation.getX()
                      + m_orientation.getY() * orientation.getY();
  const double sine   = m_orientation.getX() * orientation.getY()
                      - m_orientation.getY() * orientation.getX();
  rotateBoundForces(cosine, sine);
  m_orientation = orientation;
}

void PhysicalObject::rotate(double speed, uint64_t durationUs)
{
  if (durationUs == 0) {
    m_rotation = {};
    return;
  }
  m_rotation = {speed, durationUs};
}

void PhysicalObject::applyRotation(uint32_t intervalUs)
{
  if (m_rotation.timeLeftUs == 0 || intervalUs == 0) {
    return;
  }

  const uint64_t dtUs = intervalUs < m_rotation.timeLeftUs
      ? static_cast<uint64_t>(intervalUs)
      : m_rotation.timeLeftUs;
  m_rotation.timeLeftUs -= dtUs;

  const double angle = m_rotation.speed * (static_cast<double>(dtUs) / 1000000.0);
  if (angle == 0.0) {
    return;
  }

  const double cosine = std::cos(angle);
  const double sine = std::sin(angle);
  const double x = m_orientation.getX();
  const double y = m_orientation.getY();
  m_orientation.setPosition(x * cosine - y * sine, x * sine + y * cosine);
  m_orientation.normalize();
  rotateBoundForces(cosine, sine);
}

void PhysicalObject::rotateBoundForces(double cosine, double sine)
{
  for (Force& force : m_forces) {
    if (force.orientationBound) {
      const double x = force.vector.getX();
      const double y = force.vector.getY();
      force.vector.setPosition(x * cosine - y * sine, x * sine + y * cosine);
    }
  }
}

void PhysicalObject::moveTo(geometry::Point const& position)
{
  m_position = position;
}

void PhysicalObject::changeWeight(double delta)
{
  std::lock_guard<utils::Spinlock> guard(m_spinlock);
  if (delta < 0 && m_weight < -delta) {
    m_weight = m_minimalWeight;
    return;
  }
  m_weight += delta;
  if (m_weight < m_minimalWeight)
    m_weight = m_minimalWeight;
}

double PhysicalObject::getDistanceTo(PhysicalObject const* other)
{
  double distance = m_position.distance(other->m_position);
  distance -= std::min(distance, m_radius);
  distance -= std::min(distance, other->m_radius);
  return distance;
}

size_t PhysicalObject::allocateForce(bool orientationBound)
{
  std::lock_guard<utils::Spinlock> guard(m_spinlock);
  m_forces.emplace_back();
  m_forces.back().orientationBound = orientationBound;
  return m_forces.size() - 1;
}

} // namespace newton

#pragma once

#include <stdint.h>
#include <vector>

#include <Utils/SimpleIdPool.h>
#include <Utils/Spinlock.h>
#include <Utils/GlobalContainer.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Utils/YamlForwardDeclarations.h>
#include <World/ObjectTypes.h>

namespace world {
class Cell;
}

namespace newton {

class PhysicalObject : public utils::GlobalObject<PhysicalObject>
{
  friend class NewtonEngine;
  const double m_minimalWeight = 0.001;
public:
  struct LoadMask
  {
    enum {
      eLoadPosition    = 0x01,
      eLoadVelocity    = 0x02,
      eLoadWeight      = 0x04,
      eLoadRadius      = 0x08,
      eLoadOrientation = 0x10,
    };
    LoadMask() : nValue(0x00) {}
    LoadMask& loadPosition()    { nValue |= eLoadPosition;    return *this; }
    LoadMask& loadVelocity()    { nValue |= eLoadVelocity;    return *this; }
    LoadMask& loadWeight()      { nValue |= eLoadWeight;      return *this; }
    LoadMask& loadRadius()      { nValue |= eLoadRadius;      return *this; }
    LoadMask& loadOrientation() { nValue |= eLoadOrientation; return *this; }
    LoadMask& loadAll()         { nValue |= 0xFF; return *this; }

    uint8_t nValue;
  };

public:
  PhysicalObject(double weight, double radius);
  virtual ~PhysicalObject() = default;

  bool loadState(YAML::Node const& source, LoadMask mask = LoadMask().loadAll());

  virtual world::ObjectType getType() const {
    return world::ObjectType::ePhysicalObject;
  }

  bool is(world::ObjectType expectedType) const {
    return getType() == expectedType;
  }

  double                  getWeight()      const { return m_weight; }
  double                  getRadius()      const { return m_radius; }
  geometry::Point  const& getPosition()    const { return m_position; }
  geometry::Vector const& getVelocity()    const { return m_velocity; }
  geometry::Vector const& getOrientation() const { return m_orientation; }

  void moveTo(geometry::Point const& position);
  void setVelocity(geometry::Vector const& velocity) { m_velocity = velocity; }
  void setOrientation(geometry::Vector orientation);

  // Replace any rotation in progress. `speed` is radians per second; its sign
  // selects the direction. `durationUs` is how long the rotation lasts, in
  // microseconds. The operation keeps the time still left, not an end timestamp.
  void rotate(double speed, uint64_t durationUs);
  void changeWeight(double delta);
  void setWeight(double weight) {
    std::lock_guard<utils::Spinlock> guard(m_spinlock);
    m_weight = weight < m_minimalWeight ? m_minimalWeight : weight;
  }

  void setRadius(double radius)
  {
    assert(radius > 0);
    std::lock_guard<utils::Spinlock> guard(m_spinlock);
    m_radius = radius;
  }

  double getDistanceTo(PhysicalObject const* other);

  // A force slot lives until the object is destroyed. Fewer slots are cheaper.
  // An orientation-bound force turns with the object: the same rotation matrix
  // that turns the orientation is applied to it.
  struct Force {
    geometry::Vector vector;
    bool             orientationBound = false;
  };

  size_t allocateForce(bool orientationBound);
  geometry::Vector& getForce(size_t nForceId)
  { return m_forces[nForceId].vector; }
  geometry::Vector const& getForce(size_t nForceId) const
  { return m_forces[nForceId].vector; }

private:
  void applyRotation(uint32_t intervalUs);
  void rotateBoundForces(double cosine, double sine);

  double           m_weight;
  double           m_radius;
  geometry::Point  m_position;
  geometry::Vector m_velocity;
  geometry::Vector m_orientation;
  // `speed` is radians per second; its sign selects the direction.
  // `timeLeftUs` is the microseconds still left. Zero means no rotation.
  struct Rotation {
    double   speed = 0;
    uint64_t timeLeftUs = 0;
  } m_rotation;
  world::Cell*     m_pCell;
  utils::Spinlock  m_spinlock;

  std::vector<Force> m_forces;
};

} // namespace newton

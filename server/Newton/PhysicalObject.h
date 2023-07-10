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
      eLoadPosition = 0x01,
      eLoadVelocity = 0x02,
      eLoadWeight   = 0x04,
      eLoadRadius   = 0x08,
    };
    LoadMask() : nValue(0x00) {}
    LoadMask& loadPosition() { nValue |= eLoadPosition; return *this; }
    LoadMask& loadVelocity() { nValue |= eLoadVelocity; return *this; }
    LoadMask& loadWeight()   { nValue |= eLoadWeight;   return *this; }
    LoadMask& loadRadius()   { nValue |= eLoadRadius;   return *this; }
    LoadMask& loadAll()      { nValue |= 0xFF; return *this; }

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

  double                  getWeight()   const { return m_weight;   }
  double                  getRadius()   const { return m_radius;   }
  geometry::Point  const& getPosition() const { return m_position; }
  geometry::Vector const& getVelocity() const { return m_velocity; }

  void moveTo(geometry::Point const& position);
  void setVelocity(geometry::Vector const& velocity) { m_velocity = velocity; }
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

  // New external force will be created for object and will affect it forever!
  // There is NO WAY to remove created external force, so:
  // 1. The less forces you create, the better perfomance;
  // 2. you should store created force and change it when it is necessary.
  size_t createExternalForce();
  geometry::Vector& getExternalForce_NoSync(size_t nForceId)
  { return m_externalForces[nForceId]; }
  geometry::Vector const& getExternalForce_NoSync(size_t nForceId) const
  { return m_externalForces[nForceId]; }

private:
  double           m_weight;
  double           m_radius;
  geometry::Point  m_position;
  geometry::Vector m_velocity;
  world::Cell*     m_pCell;
  utils::Spinlock  m_spinlock;

  // Number of external forces
  std::vector<geometry::Vector> m_externalForces;
};

} // namespace newton

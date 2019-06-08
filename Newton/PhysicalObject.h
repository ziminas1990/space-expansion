#pragma once

#include <stdint.h>
#include <vector>

#include <Utils/SimplePool.h>
#include <Utils/Spinlock.h>
#include <Utils/GlobalContainer.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Utils/YamlForwardDeclarations.h>

namespace newton {

class PhysicalObject : public utils::GlobalContainer<PhysicalObject>
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

  bool loadState(YAML::Node const& source, LoadMask mask = LoadMask().loadAll());

  double                  getWeight()   const { return m_weight;   }
  geometry::Point  const& getPosition() const { return m_position; }
  geometry::Vector const& getVelocity() const { return m_velocity; }

  void changeWeight(double delta);
  void setWeight(double weight) {
    // May be inlining this function is better
    std::lock_guard<utils::Spinlock> guard(m_spinlock);
    m_weight = weight < m_minimalWeight ? m_minimalWeight : weight;
  }

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
  utils::Spinlock  m_spinlock;

  // Number of external forces
  std::vector<geometry::Vector> m_externalForces;
};

} // namespace newton

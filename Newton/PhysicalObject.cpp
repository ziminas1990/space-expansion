#include "PhysicalObject.h"

#include <mutex>  // for std::lock_guard
#include <Utils/YamlReader.h>

DECLARE_GLOBAL_CONTAINER_CPP(newton::PhysicalObject);

namespace newton {

PhysicalObject::PhysicalObject(double weight)
{
  GlobalContainer<PhysicalObject>::registerSelf(this);
  setWeight(weight);
  m_externalForces.reserve(4);
}

bool PhysicalObject::loadState(YAML::Node const& source)
{
  return utils::YamlReader(source)
      .read("weight", m_weight)
      .read("position", m_position)
      .read("velocity", m_velocity);
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

size_t PhysicalObject::createExternalForce()
{
  std::lock_guard<utils::Spinlock> guard(m_spinlock);
  m_externalForces.emplace_back();
  return m_externalForces.size() - 1;
}

} // namespace newton

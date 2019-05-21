#include "PhysicalObject.h"

#include <mutex>  // for std::lock_guard

namespace newton {

utils::ThreadSafePool<uint32_t> PhysicalObject::m_IdPool;
std::vector<PhysicalObject*>    PhysicalObject::m_allPhysicalObjects;

PhysicalObject::PhysicalObject(double weight)
  : m_nId(m_IdPool.getNext())
{
  setWeight(weight);
  m_externalForces.reserve(4);
  if (m_nId >= m_allPhysicalObjects.size()) {
    if (m_allPhysicalObjects.empty())
      m_allPhysicalObjects.reserve(1024);
    m_allPhysicalObjects.resize(m_nId + 1);
  }
  m_allPhysicalObjects[m_nId] = this;
}

PhysicalObject::~PhysicalObject()
{
  m_allPhysicalObjects[m_nId] = nullptr;
  m_IdPool.release(m_nId);
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
  return  m_externalForces.size() - 1;
}

} // namespace newton

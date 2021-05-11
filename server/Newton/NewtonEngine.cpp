#include "NewtonEngine.h"

#include <mutex>
#include <algorithm>

namespace newton
{

bool NewtonEngine::prephare(uint16_t, uint32_t, uint64_t)
{
  m_nNextObjectId.store(0);
  return true;
}

void NewtonEngine::proceed(uint16_t, uint32_t nIntervalUs, uint64_t)
{
  const uint32_t step = 256;
  double nIntervalSec = nIntervalUs / 1000000.0;

  uint32_t nId = m_nNextObjectId.fetch_add(step);
  uint32_t end = std::min(nId + step, PhysicalObject::TotalInstancies());
  while (nId < end) {
    PhysicalObject* pObject = PhysicalObject::Instance(nId);
    if (pObject) {
      // acc_t - acceleration * time
      geometry::Vector acc_t;
      for (geometry::Vector const& externalForce : pObject->m_externalForces)
        acc_t += externalForce;
      acc_t *= nIntervalSec/pObject->m_weight;

      geometry::Vector movement(pObject->m_velocity, nIntervalSec);
      movement.add(acc_t, nIntervalSec * 0.5);
      pObject->m_position.translate(movement);
      pObject->m_velocity += acc_t;
    }
    nId = m_nNextObjectId.fetch_add(step);
    end = std::min(nId + step, PhysicalObject::TotalInstancies());
  }
}


} // namespace newton

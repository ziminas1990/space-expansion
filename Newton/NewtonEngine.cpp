#include "NewtonEngine.h"

#include <mutex>

namespace newton
{

bool NewtonEngine::prephareStage(uint16_t)
{
  m_nNextObjectId.store(0);
  return true;
}

void NewtonEngine::proceedStage(uint16_t, uint32_t nIntervalUs)
{
  double nIntervalSec = nIntervalUs / 1000000.0;

  size_t nId = m_nNextObjectId.fetch_add(1);
  while (nId < PhysicalObject::TotalInstancies()) {
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
    nId = m_nNextObjectId.fetch_add(1);
  }
}


} // namespace newton

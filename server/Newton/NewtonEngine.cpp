#include "NewtonEngine.h"
#include <World/Grid.h>

#include <mutex>
#include <algorithm>

namespace newton
{

using AllObjects = utils::GlobalContainer<PhysicalObject>;

bool NewtonEngine::prephare(uint16_t, uint32_t, uint64_t)
{
  m_nNextObjectId.store(0);
  return true;
}

void NewtonEngine::proceed(uint16_t, uint32_t nIntervalUs, uint64_t)
{
  const uint32_t step = 64;
  const double nIntervalSec = nIntervalUs / 1000000.0;

  uint32_t begin = m_nNextObjectId.fetch_add(step);
  while (begin < AllObjects::Total()) {
    const uint32_t end = std::min(begin + step, AllObjects::Total());
    for (uint32_t nId = begin; nId < end; ++nId) {
      PhysicalObject* pObject = AllObjects::Instance(nId);
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

        if (pObject->m_pCell) {
          pObject->m_pCell = pObject->m_pCell->track(
                pObject->getInstanceId(),
                pObject->m_position.x,
                pObject->m_position.y);
        } else {
          pObject->m_pCell = world::Grid::getGlobal()->add(
                pObject->getInstanceId(),
                pObject->m_position.x,
                pObject->m_position.y);
        }
      }
    }
    begin = m_nNextObjectId.fetch_add(step);
  }
}


} // namespace newton

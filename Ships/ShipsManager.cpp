#include "ShipsManager.h"

#include "Ship.h"
#include <utility>
#include <Utils/WeakPtrs.h>

namespace ships {

ShipsManager::ShipsManager()
  : m_nRemovingZombiesTimeout(100)
{}

void ShipsManager::addNewOne(ShipWeakPtr pShip)
{
  std::lock_guard<utils::Mutex> guard(m_Mutex);
  m_Ships.push_back(pShip);
}

bool ShipsManager::prephareStage(uint16_t /*nStageId*/)
{
  if (--m_nRemovingZombiesTimeout == 0) {
    // sometimes we should remove empty weak_ptr's from m_CommandCentres
    utils::removeExpiredWeakPtrs(m_Ships);
    // TODO: change to rand()?
    m_nRemovingZombiesTimeout = 100;
  }

  m_nNextId.store(0);
  return !m_Ships.empty();
}

void ShipsManager::proceedStage(uint16_t /*nStageId*/, uint32_t /*nIntervalUs*/)
{
  size_t nId = m_nNextId.fetch_add(1);
  while (nId < m_Ships.size()) {
    ShipPtr pCommandCenter = m_Ships[nId].lock();
    if (!pCommandCenter) {
      m_Ships[nId].reset();
    }
    pCommandCenter->handleBufferedMessages();
    nId = m_nNextId.fetch_add(1);
  }
}


} // namespace modules

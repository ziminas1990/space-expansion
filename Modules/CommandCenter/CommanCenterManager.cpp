#include "CommanCenterManager.h"

#include <utility>
#include "CommandCenter.h"
#include <Utils/WeakPtrs.h>

namespace modules {

CommandCenterManager::CommandCenterManager()
  : m_nRemovingZombiesTimeout(100)
{}

void CommandCenterManager::addNewOne(CommandCenterWeakPtr pCommandCenter)
{
  std::lock_guard<std::mutex> guard(m_Mutex);
  m_CommandCentres.push_back(pCommandCenter);
}

bool CommandCenterManager::prephareStage(uint16_t /*nStageId*/)
{
  if (--m_nRemovingZombiesTimeout == 0) {
    // sometimes we should remove empty weak_ptr's from m_CommandCentres
    utils::removeExpiredWeakPtrs(m_CommandCentres);
    // TODO: change to rand()?
    m_nRemovingZombiesTimeout = 100;
  }

  m_nNextId.store(0);
  return !m_CommandCentres.empty();
}

void CommandCenterManager::proceedStage(uint16_t /*nStageId*/, uint32_t /*nIntervalUs*/)
{
  size_t nId = m_nNextId.fetch_add(1);
  while (nId < m_CommandCentres.size()) {
    CommandCenterPtr pCommandCenter = m_CommandCentres[nId].lock();
    if (!pCommandCenter) {
      m_CommandCentres[nId].reset();
    }
    pCommandCenter->handleBufferedMessages();
    nId = m_nNextId.fetch_add(1);
  }
}


} // namespace modules

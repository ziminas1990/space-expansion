#include "CommanCenterManager.h"

#include <utility>
#include "CommandCenter.h"

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
    for(size_t i = 0; i < m_CommandCentres.size(); ++i) {
      if (m_CommandCentres[i].expired()) {
        // To remove element, just swap it with last element and than remove last element
        if (i < m_CommandCentres.size() - 1)
          std::swap(m_CommandCentres.at(i), m_CommandCentres.back());
        m_CommandCentres.pop_back();
      }
    }
    m_nRemovingZombiesTimeout = 100;
  }

  m_nNextId.store(0);
  return !m_CommandCentres.empty();
}

void CommandCenterManager::proceedStage(uint16_t /*nStageId*/, size_t /*nIntervalUs*/)
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

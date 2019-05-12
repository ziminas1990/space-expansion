#include "CommutatorManager.h"
#include <mutex>

namespace modules
{

CommutatorManager::CommutatorManager()
{
  m_commutators.reserve(0xFF);
}

CommutatorPtr CommutatorManager::makeCommutator()
{
  CommutatorPtr pCommutator = std::make_shared<Commutator>();
  {
    std::lock_guard<utils::Mutex> lock(m_AccessLock);
    m_commutators.push_back(pCommutator);
  }
  return pCommutator;
}

bool CommutatorManager::prephareStage(uint16_t)
{
  if (m_lInactiveCommutatorDetected.test_and_set()) {
    removeInactiveCommutators();
  }
  m_lInactiveCommutatorDetected.clear();
  m_nNextId.store(0);
  return true;
}

void CommutatorManager::proceedStage(uint16_t, uint32_t)
{
  for (size_t nCommutatorId = m_nNextId.fetch_add(1);
       nCommutatorId < m_commutators.size();
       nCommutatorId = m_nNextId.fetch_add(1))
  {
    CommutatorPtr& pCommutator = m_commutators[nCommutatorId];
    if (pCommutator && pCommutator->isOnline()) {
      pCommutator->handleBufferedMessages();
    } else {
      m_lInactiveCommutatorDetected.test_and_set();
    }
  }
}

void CommutatorManager::removeInactiveCommutators()
{
  for(size_t i = 0; i < m_commutators.size(); ++i) {
    CommutatorPtr& pCommutator = m_commutators[i];
    if (!pCommutator || pCommutator->isDestroyed()) {
      if (i < m_commutators.size() - 1)
        std::swap(m_commutators.at(i), m_commutators.back());
      m_commutators.pop_back();
    }
  }
}

} // namespace modules

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
    std::lock_guard<utils::SpinLock> lock(m_AccessLock);
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

void CommutatorManager::proceedStage(uint16_t, size_t)
{
  size_t nCommutatorId = m_nNextId.fetch_add(1);
  while(nCommutatorId < m_commutators.size()) {
    CommutatorPtr& pCommutator = m_commutators[nCommutatorId];
    if (pCommutator && pCommutator->getStatus() != Commutator::eDestoyed) {
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
    if (!pCommutator || pCommutator->getStatus() == Commutator::eDestoyed) {
      if (i < m_commutators.size() - 1)
        std::swap(m_commutators.at(i), m_commutators.back());
      m_commutators.pop_back();
    }
  }
}

} // namespace modules

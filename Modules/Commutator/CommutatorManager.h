#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include "Commutator.h"
#include <Conveyor/IAbstractLogic.h>
#include <Utils/Mutex.h>

namespace modules
{

class CommutatorManager : public conveyor::IAbstractLogic
{
public:
  CommutatorManager();

  CommutatorPtr makeCommutator();

  // IAbstractLogic interface
  uint16_t getStagesCount() { return 1; }
  bool     prephareStage(uint16_t nStageId);
  void     proceedStage(uint16_t nStageId, uint32_t nIntervalUs);
  size_t   getCooldownTimeUs() const { return 1000; }

private:
  void removeInactiveCommutators();

private:
  std::vector<CommutatorPtr> m_commutators;
  std::atomic_size_t         m_nNextId;
  std::atomic_flag           m_lInactiveCommutatorDetected = ATOMIC_FLAG_INIT;

  utils::Mutex m_AccessLock;
};

using CommutatorManagerPtr = std::shared_ptr<CommutatorManager>;

} // namespace modules

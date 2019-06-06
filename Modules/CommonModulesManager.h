#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <Conveyor/IAbstractLogic.h>
#include <Utils/GlobalContainer.h>

namespace modules
{

// Common manager for any subclass of BaseModule class
// Type ModuleType:
// 1. should be inherited from BaseModule
// 2. should be inherited from utils::GlobalContainer<ModuleType>

template <typename ModuleType, size_t nCooldownTimeUs = 10000>
class CommonModulesManager : public conveyor::IAbstractLogic
{
public:
  // IAbstractLogic interface
  uint16_t getStagesCount() { return 1; }
  bool prephareStage(uint16_t)
  {
    m_nNextId = 0;
    return true;
  }

  void proceedStage(uint16_t, uint32_t)
  {
    size_t nId = m_nNextId.fetch_add(1);
    for (; nId < utils::GlobalContainer<ModuleType>::TotalInstancies();
         nId = m_nNextId.fetch_add(1))
    {
      ModuleType* pObject = utils::GlobalContainer<ModuleType>::Instance(nId);
      if (!pObject || !pObject->isOnline())
        continue;
      pObject->handleBufferedMessages();
    }
  }

  size_t getCooldownTimeUs() const { return nCooldownTimeUs; }

private:
  std::atomic_size_t m_nNextId;
};

} // namespace modules

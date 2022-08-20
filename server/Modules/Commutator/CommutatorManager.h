#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <Conveyor/IAbstractLogic.h>
#include <Utils/GlobalContainer.h>
#include <Utils/IdArray.h>
#include <Modules/Commutator/Commutator.h>

namespace modules
{


// Common manager for any subclass of BaseModule class
// Type ModuleType:
// 1. should be inherited from BaseModule
// 2. should be inherited from utils::GlobalContainer<ModuleType>

class CommutatorManager : public conveyor::IAbstractLogic
{
  enum Stages {
    eStageHandleMessages = 0,
    eCheckSlots          = 1,
    eTotalStages         = 2
  };

public:
  // IAbstractLogic interface
  uint16_t getStagesCount() { return eTotalStages; }

  bool prephare(uint16_t nStageId, uint32_t, uint64_t nNowUs)
  {
    switch (nStageId) {
      case eStageHandleMessages:
        m_nNextId = 0;
        return !utils::GlobalContainer<Commutator>::Empty();
      case eCheckSlots:
        m_nNextId = 0;
        return nNowUs > m_nLastSlotsCheckUs + 25000;
      default: {
        return false;
      }
    }
  }

  void proceed(uint16_t nStageId, uint32_t, uint64_t nNowUs)
  {
    switch (nStageId) {
      case eStageHandleMessages:
        handleAllMessages();
        return;
      case eCheckSlots:
        checkSlots(nNowUs);
        return;
      case eTotalStages: {
        return;
      }
    }
  }

  size_t getCooldownTimeUs() const { 
    // Commutator should handle all requests immdiatelly
    return 0;
  }

private:
  void handleAllMessages()
  {
    const uint32_t nTotalModules =
        utils::GlobalContainer<Commutator>::Size();
    uint32_t nId = static_cast<uint32_t>(m_nNextId.fetch_add(1));
    for (; nId < nTotalModules;
         nId = static_cast<uint32_t>(m_nNextId.fetch_add(1)))
    {
      BaseModule* pModule = utils::GlobalContainer<Commutator>::Instance(nId);
      if (pModule) {
        pModule->handleBufferedMessages();
      }
    }
  }

  void checkSlots(uint64_t nNowUs)
  {
    m_nLastSlotsCheckUs = nNowUs;
    const uint32_t nTotalModules =
        utils::GlobalContainer<Commutator>::Size();
    uint32_t nId = static_cast<uint32_t>(m_nNextId.fetch_add(1));
    for (; nId < nTotalModules;
         nId = static_cast<uint32_t>(m_nNextId.fetch_add(1)))
    {
      Commutator* pModule = utils::GlobalContainer<Commutator>::Instance(nId);
      if (pModule) {
        pModule->checkSlots();
      }
    }
  }

private:
  // When slots where checked last time
  uint64_t m_nLastSlotsCheckUs = 0;

  std::atomic_size_t m_nNextId;
};

} // namespace modules

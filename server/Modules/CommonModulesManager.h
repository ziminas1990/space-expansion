#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include <Conveyor/IAbstractLogic.h>
#include <Utils/GlobalContainer.h>
#include <Utils/IdArray.h>
#include "BaseModule.h"

namespace modules
{

#ifndef AUTOTESTS_MODE
enum class Cooldown {
  eSystemClock       =      0,
  eCommutator        =      0,
  eDefault           =  10101,
  eShip              =  10202,
  eEngine            =  30303,
  ePassiveScanner    =  50405,
  eMessanger         =  50507,
  eAsteroidMiner     = 150611,
  eAsteroidScanner   = 150713,
  eShipyard          = 150817,
  eCelestialScanner  = 200919,
  eResourceContainer = 201023,
  eBlueprintsStorage = 201129,
};
#else
// In autotests mode we can't afford to let logics to sleep for unpredictable
// period of time
enum class Cooldown {
  eSystemClock       = 0,
  eCommutator        = 0,
  eDefault           = 0,
  eShip              = 0,
  eEngine            = 0,
  eAsteroidScanner   = 0,
  eAsteroidMiner     = 0,
  eCelestialScanner  = 0,
  ePassiveScanner    = 50405,
  eMessanger         = 50507,
  eShipyard          = 150817,
  eResourceContainer = 201023,
  eBlueprintsStorage = 201129
};
#endif

// Common manager for any subclass of BaseModule class
// Type ModuleType:
// 1. should be inherited from BaseModule
// 2. should be inherited from utils::GlobalContainer<ModuleType>

template <typename ModuleType, Cooldown nCooldown = Cooldown::eDefault>
class CommonModulesManager : public conveyor::IAbstractLogic
{
  enum Stages {
    eStageHandleMessages = 0,
    eStageProceeding     = 1,
    eTotalStages         = 2
  };

public:
  // IAbstractLogic interface
  uint16_t getStagesCount() { return eTotalStages; }

  bool prephare(uint16_t nStageId, uint32_t nIntervalUs, uint64_t nNowUs)
  {
    switch (nStageId) {
      case eStageHandleMessages:
        m_nNextId = 0;
        return !utils::GlobalContainer<ModuleType>::Empty();
      case eStageProceeding:
        m_busyModulesIds.begin();
        return !m_busyModulesIds.empty();
      case eTotalStages: {
        // [[fallthrough]];
      }
    }
    return prepareAdditionalStage(
          nStageId - eTotalStages, nIntervalUs, nNowUs);
  }

  void proceed(uint16_t nStageId, uint32_t nIntervalUs, uint64_t nNowUs)
  {
    switch (nStageId) {
      case eStageHandleMessages:
        handleAllMessages();
        return;
      case eStageProceeding:
        proceedBusyModules(nIntervalUs);
        return;
      case eTotalStages: {
        // [[fallthrough]];
      }
    }
    proceedAdditionalStage(nStageId - eTotalStages, nIntervalUs, nNowUs);
  }

  size_t getCooldownTimeUs() const { return static_cast<size_t>(nCooldown); }

protected:
  virtual bool prepareAdditionalStage([[maybe_unused]] uint16_t nStageId,
                                      [[maybe_unused]] uint32_t nIntervalUs,
                                      [[maybe_unused]] uint64_t nNowUs) {
    // Will be called in case inheriter overwrites 'getStagesCount()' and
    // it returns a number greater then 'eTotalStages'
    assert(!"If 'getStagesCount()' is overwritten, you must also provide"
            " corresponding 'prepareAdditionalStage()' implementation!");
    return false;
  }

  virtual void proceedAdditionalStage([[maybe_unused]] uint16_t nStageId,
                                      [[maybe_unused]] uint32_t nIntervalUs,
                                      [[maybe_unused]] uint64_t nNowUs) {
    assert(!"If 'getStagesCount()' is overwritten, you must also provide"
            " corresponding 'proceedAdditionalStage()' implementation!");
  }

private:
  void handleAllMessages()
  {
    // This function just call handleBufferedMessages() on every module instance
    // If module switched state from Idle to Busy, it will adds module to the list of
    // busy modules (it will be proceeded until is switches to Idle state)

    const uint32_t nTotalModules =
        utils::GlobalContainer<ModuleType>::Size();
    uint32_t nId = static_cast<uint32_t>(m_nNextId.fetch_add(1));
    for (; nId < nTotalModules;
         nId = static_cast<uint32_t>(m_nNextId.fetch_add(1)))
    {
      BaseModule* pModule = utils::GlobalContainer<ModuleType>::Instance(nId);
      if (pModule) {
        pModule->handleBufferedMessages();
        if (pModule->isActivating()) {
          m_busyModulesIds.push(nId);
          pModule->onActivated();
        }
      }
    }
  }

  void proceedBusyModules(uint32_t nIntervalUs)
  {
    // This function proceed every busy module and remove idle modules from
    // list of busy modules

    uint32_t nModuleId = 0;
    size_t   nIndex    = 0;
    while (m_busyModulesIds.getNextId(nModuleId, nIndex)) {
      BaseModule* pModule = utils::GlobalContainer<ModuleType>::Instance(nModuleId);
      if (!pModule) {
        m_busyModulesIds.dropIndex(nIndex);
        continue;
      }
      pModule->proceed(nIntervalUs);
      if (pModule->isDeactivating()) {
        m_busyModulesIds.dropIndex(nIndex);
        pModule->onDeactivated();
      }
    }
  }

private:
  utils::IdArray<uint32_t> m_busyModulesIds;

  std::atomic_size_t m_nNextId;
};

} // namespace modules

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
  eDefault           =  10100,
  eShip              =  10200,
  eEngine            =  30300,
  eAsteroidMiner     = 154000,
  eAsteroidScanner   = 155000,
  eCelestialScanner  = 206000,
  eResourceContainer = 307000,
  eBlueprintsStorage = 308000,
  eShipYard          = 309000,
};
#else
// In autotests mode we can't afford to let logics to sleep for unpredictable period
// of time
enum class Cooldown {
  eSystemClock       = 0,
  eCommutator        = 0,
  eDefault           = 0,
  eShip              = 0,
  eEngine            = 0,
  eAsteroidScanner   = 0,
  eAsteroidMiner     = 0,
  eCelestialScanner  = 0,
  eResourceContainer = 307000,
  eBlueprintsStorage = 308000,
  eShipYard          = 309000,
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
  bool prephare(uint16_t nStageId, uint32_t, uint64_t)
  {
    switch (nStageId) {
      case eStageHandleMessages:
        m_nNextId = 0;
        return !utils::GlobalContainer<ModuleType>::empty();
      case eStageProceeding:
        m_busyModulesIds.begin();
        return !m_busyModulesIds.empty();
      default:
        assert(false);
		return false;
    }
  }

  void proceed(uint16_t nStageId, uint32_t nIntervalUs, uint64_t)
  {
    switch (nStageId) {
      case eStageHandleMessages:
        handleAllMessages();
        break;
      case eStageProceeding:
        proceedBusyModules(nIntervalUs);
        break;
      default:
        assert(false);
    }
  }

  size_t getCooldownTimeUs() const { return static_cast<size_t>(nCooldown); }

private:
  void handleAllMessages()
  {
    // This function just call handleBufferedMessages() on every module instance
    // If module switched state from Idle to Busy, it will adds module to the list of
    // busy modules (it will be proceeded until is switches to Idle state)

    size_t nId = m_nNextId.fetch_add(1);
    for (; nId < utils::GlobalContainer<ModuleType>::TotalInstancies();
         nId = m_nNextId.fetch_add(1))
    {
      BaseModule* pModule = utils::GlobalContainer<ModuleType>::Instance(nId);
      if (!pModule || !pModule->isOnline())
        continue;
      pModule->handleBufferedMessages();
      if (pModule->isActivating()) {
        m_busyModulesIds.push(nId);
        pModule->onActivated();
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
        m_busyModulesIds.forgetIdAtPosition(nIndex);
        continue;
      }
      pModule->proceed(nIntervalUs);
      if (pModule->isDeactivating()) {
        m_busyModulesIds.forgetIdAtPosition(nIndex);
        pModule->onDeactivated();
      }
    }
  }

private:
  utils::IdArray<uint32_t> m_busyModulesIds;

  std::atomic_size_t m_nNextId;
};

} // namespace modules

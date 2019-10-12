#include "FindModule.h"

namespace autotests { namespace client {

bool GetAllModules(Ship &ship, std::string const& sModuleType, ModulesList &modules)
{
  uint32_t nTotalSlots;
  if (!ship.getTotalSlots(nTotalSlots) || !nTotalSlots)
    return false;

  ModulesList attachedModules;
  if (!ship.getAttachedModulesList(nTotalSlots, attachedModules))
    return false;

  for (ModuleInfo const& module : attachedModules) {
    if (module.sModuleType == sModuleType)
      modules.push_back(module);
  }
  return true;
}

bool FindMostPowerfulEngine(Ship& ship, Engine& mostPowerfullEngine)
{
  ModulesList engines;
  if (!GetAllModules(ship, "Engine/Nuclear", engines))
    return false;
  if (engines.empty())
    return false;

  uint32_t  nMaxThrust = 0;

  for (ModuleInfo const& engineInfo : engines) {
    TunnelPtr pTunnel = ship.openTunnel(engineInfo.nSlotId);
    if (!pTunnel)
      return false;

    Engine engine;
    engine.attachToChannel(pTunnel);

    EngineSpecification specification;
    if (!engine.getSpecification(specification))
      return false;
    if (specification.nMaxThrust > nMaxThrust) {
      mostPowerfullEngine.attachToChannel(pTunnel);
    }
  }
  return true;
}

}}  // namespace autotests::client

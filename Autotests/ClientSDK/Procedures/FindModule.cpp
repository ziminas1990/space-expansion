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
    std::string sType = module.sModuleType.substr(0, module.sModuleType.find("/"));
    if (sType == sModuleType)
      modules.push_back(module);
  }
  return true;
}

bool FindMostPowerfulEngine(Ship& ship, Engine& mostPowerfullEngine)
{
  ModulesList engines;
  if (!GetAllModules(ship, "Engine", engines))
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

bool FindBestCelestialScanner(Ship &ship, CelestialScanner& bestScanner,
                              CelestialScannerSpecification *pSpec)
{
  ModulesList scanners;
  if (!GetAllModules(ship, "CelestialScanner", scanners))
    return false;
  if (scanners.empty())
    return false;

  uint32_t nMaxScanningRadiusKm = 0;

  for (ModuleInfo const& moduleInfo : scanners) {
    TunnelPtr pTunnel = ship.openTunnel(moduleInfo.nSlotId);
    if (!pTunnel)
      return false;

    CelestialScanner scanner;
    scanner.attachToChannel(pTunnel);

    CelestialScannerSpecification specification;
    if (!scanner.getSpecification(specification))
      return false;
    if (specification.m_nMaxScanningRadiusKm > nMaxScanningRadiusKm) {
      bestScanner.attachToChannel(pTunnel);
      if (pSpec) {
        *pSpec = specification;
      }
    }
  }
  return true;
}

bool FindSomeAsteroidScanner(Ship& ship, AsteroidScanner& someScanner)
{
  ModulesList scanners;
  if (!GetAllModules(ship, "AsteroidScanner", scanners))
    return false;
  if (scanners.empty())
    return false;

  ModuleInfo const& moduleInfo = scanners.front();
  TunnelPtr pTunnel = ship.openTunnel(moduleInfo.nSlotId);
  if (!pTunnel)
    return false;
  someScanner.attachToChannel(pTunnel);
  return true;
}

}}  // namespace autotests::client

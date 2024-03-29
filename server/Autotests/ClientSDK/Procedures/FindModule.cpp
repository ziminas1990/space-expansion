#include "FindModule.h"

namespace autotests { namespace client {

bool attachToShip(ClientCommutatorPtr pRootCommutator, std::string const& sShipName,
                  Ship &ship)
{
  ModulesList ships;
  if (!pRootCommutator->getAttachedModulesList(ships))
    return false;

  for (ModuleInfo& shipInfo : ships) {
    if (shipInfo.sModuleName == sShipName) {
      client::Router::SessionPtr pSession = 
          pRootCommutator->openSession(shipInfo.nSlotId);
      ship.attachToChannel(pSession);
      return pSession != nullptr;
    }
  }
  return false;
}

bool GetAllModules(ClientCommutator& commutator,
                   std::string const& sModuleType,
                   ModulesList &modules)
{
  ModulesList attachedModules;
  if (!commutator.getAttachedModulesList(attachedModules))
    return false;

  for (ModuleInfo const& module : attachedModules) {
    std::string sType = module.sModuleType.substr(0, module.sModuleType.find("/"));
    if (sType == sModuleType)
      modules.push_back(module);
  }
  return true;
}


bool FindModule(ClientCommutator&  commutator,
                std::string const& sModuleClass,
                ClientBaseModule&  module,
                std::string const& sName)
{
  ModulesList containers;
  if (!GetAllModules(commutator, sModuleClass, containers))
    return false;
  if (containers.empty())
    return false;

  for (ModuleInfo const& moduleInfo : containers) {
    if (!sName.empty() && moduleInfo.sModuleName != sName)
      continue;

    client::Router::SessionPtr pSession = 
        commutator.openSession(moduleInfo.nSlotId);
    module.attachToChannel(pSession);
    return pSession != nullptr;
  }
  return false;
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

    client::Router::SessionPtr pSession = 
        ship.openSession(engineInfo.nSlotId);
    if (!pSession)
      return false;

    Engine engine;
    engine.attachToChannel(pSession);

    EngineSpecification specification;
    if (!engine.getSpecification(specification))
      return false;
    if (specification.nMaxThrust > nMaxThrust) {
      mostPowerfullEngine.attachToChannel(pSession);
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
    client::Router::SessionPtr pSession = ship.openSession(moduleInfo.nSlotId);
    if (!pSession)
      return false;

    CelestialScanner scanner;
    scanner.attachToChannel(pSession);

    CelestialScannerSpecification specification;
    if (!scanner.getSpecification(specification))
      return false;
    if (specification.m_nMaxScanningRadiusKm > nMaxScanningRadiusKm) {
      bestScanner.attachToChannel(pSession);
      if (pSpec) {
        *pSpec = specification;
      }
    }
  }
  return true;
}

bool FindAsteroidScanner(Ship& ship, AsteroidScanner& scanner, std::string const& sName)
{
  return FindModule(ship, "AsteroidScanner", scanner, sName);
}

bool FindResourceContainer(Ship &ship, ResourceContainer &container,
                           std::string const& sName)
{
  return FindModule(ship, "ResourceContainer", container, sName);
}

bool FindAsteroidMiner(Ship &ship, AsteroidMiner& miner, std::string const& sName)
{
  return FindModule(ship, "AsteroidMiner", miner, sName);
}

bool FindBlueprintStorage(ClientCommutator& commutator, BlueprintsStorage& storage,
                          std::string const& sName)
{
  return FindModule(commutator, "BlueprintsLibrary", storage, sName);
}

bool FindShipyard(ClientCommutator &commutator, Shipyard& shipyard,
                  std::string const& sName)
{
  return FindModule(commutator, "Shipyard", shipyard, sName);
}


}}  // namespace autotests::client

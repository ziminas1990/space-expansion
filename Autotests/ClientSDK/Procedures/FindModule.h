#pragma once

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientEngine.h>
#include <Autotests/ClientSDK/Modules/ClientCelestialScanner.h>
#include <Autotests/ClientSDK/Modules/ClientAsteroidScanner.h>
#include <Autotests/ClientSDK/Modules/ClientResourceContainer.h>
#include <Autotests/ClientSDK/Modules/ClientAsteroidMiner.h>
#include <Autotests/ClientSDK/Modules/ClientBlueprintStorage.h>

namespace autotests { namespace client {

bool attachToShip(ClientCommutatorPtr pRootCommutator, std::string const& sShipName,
                  Ship& ship);

bool GetAllModules(ClientCommutator& commutator, std::string const& sModuleType,
                   ModulesList& modules);

bool FindModule(ClientCommutator&  commutator,
                std::string const& sModuleClass,
                ClientBaseModule&  module,
                std::string const& sName = std::string());

bool FindMostPowerfulEngine(Ship& ship, Engine& mostPowerfullEngine);
bool FindBestCelestialScanner(Ship &ship, CelestialScanner& bestScanner,
                              CelestialScannerSpecification* pSpec = nullptr);
bool FindAsteroidScanner(Ship& ship, AsteroidScanner& scanner,
                         std::string const& sName = std::string());
bool FindResourceContainer(Ship& ship, ResourceContainer& container,
                           std::string const& sName = std::string());
bool FindAsteroidMiner(Ship& ship, AsteroidMiner& miner,
                       std::string const& sName = std::string());
bool FindBlueprintStorage(ClientCommutator &commutator, BlueprintsStorage& storage,
                          std::string const& sName = std::string());

}}  // namespace autotests::client

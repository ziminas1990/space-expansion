#pragma once

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientEngine.h>
#include <Autotests/ClientSDK/Modules/ClientCelestialScanner.h>

namespace autotests { namespace client {

bool GetAllModules(Ship& ship, std::string const& sModuleType,
                   ModulesList& modules);

bool FindMostPowerfulEngine(Ship& ship, Engine& mostPowerfullEngine);
bool FindBestCelestialScanner(Ship &ship, CelestialScanner& bestScanner,
                              CelestialScannerSpecification* pSpec = nullptr);

}}  // namespace autotests::client

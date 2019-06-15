#pragma once

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientEngine.h>

namespace autotests { namespace client {

bool GetAllModules(ClientShip& ship, std::string const& sModuleType,
                   ModulesList& modules);

bool FindMostPowerfulEngine(ClientShip& ship, Engine& mostPowerfullEngine);

}}  // namespace autotests::client

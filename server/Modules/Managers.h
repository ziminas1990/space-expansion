#pragma once

#include <Modules/All.h>
#include <Modules/CommonModulesManager.h>
#include <Modules/ResourceContainer/ResourceContainerManager.h>
#include <Modules/Commutator/CommutatorManager.h>

#define DECLARE_DEFAULT_MODULE_MANAGER(ModuleCls) \
  class ModuleCls##Manager\
  : public CommonModulesManager<ModuleCls, Cooldown::e##ModuleCls>\
  {};

namespace modules
{

DECLARE_DEFAULT_MODULE_MANAGER(AsteroidMiner)
DECLARE_DEFAULT_MODULE_MANAGER(AsteroidScanner)
DECLARE_DEFAULT_MODULE_MANAGER(BlueprintsStorage)
DECLARE_DEFAULT_MODULE_MANAGER(CelestialScanner)
DECLARE_DEFAULT_MODULE_MANAGER(Engine)
DECLARE_DEFAULT_MODULE_MANAGER(PassiveScanner)
DECLARE_DEFAULT_MODULE_MANAGER(Shipyard)
DECLARE_DEFAULT_MODULE_MANAGER(Ship)
DECLARE_DEFAULT_MODULE_MANAGER(SystemClock)
DECLARE_DEFAULT_MODULE_MANAGER(Messanger)

} // namespace modules

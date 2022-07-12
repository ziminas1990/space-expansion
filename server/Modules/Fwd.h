#pragma once

#include <memory>

#define MODULE_FWD_DECLARATION(ModuleCls) \
  class ModuleCls; \
  using ModuleCls##Ptr = std::shared_ptr<ModuleCls>; \
  using ModuleCls##WeakPtr = std::weak_ptr<ModuleCls>; \
  

#define MODULE_MANAGER_FWD_DECLARATION(ModuleCls) \
  class ModuleCls##Manager; \
  using ModuleCls##ManagerPtr = std::shared_ptr<ModuleCls##Manager>;


namespace modules {

MODULE_FWD_DECLARATION(BaseModule)
MODULE_FWD_DECLARATION(AccessPanel)
MODULE_FWD_DECLARATION(AsteroidMiner)
MODULE_FWD_DECLARATION(AsteroidScanner)
MODULE_FWD_DECLARATION(BlueprintsStorage)
MODULE_FWD_DECLARATION(CelestialScanner)
MODULE_FWD_DECLARATION(Commutator)
MODULE_FWD_DECLARATION(Engine)
MODULE_FWD_DECLARATION(PassiveScanner)
MODULE_FWD_DECLARATION(ResourceContainer)
MODULE_FWD_DECLARATION(Ship)
MODULE_FWD_DECLARATION(Shipyard)
MODULE_FWD_DECLARATION(SystemClock)

MODULE_MANAGER_FWD_DECLARATION(AsteroidMiner)
MODULE_MANAGER_FWD_DECLARATION(AsteroidScanner)
MODULE_MANAGER_FWD_DECLARATION(BlueprintsStorage)
MODULE_MANAGER_FWD_DECLARATION(CelestialScanner)
MODULE_MANAGER_FWD_DECLARATION(Commutator)
MODULE_MANAGER_FWD_DECLARATION(Engine)
MODULE_MANAGER_FWD_DECLARATION(PassiveScanner)
MODULE_MANAGER_FWD_DECLARATION(ResourceContainer)
MODULE_MANAGER_FWD_DECLARATION(Ship)
MODULE_MANAGER_FWD_DECLARATION(Shipyard)
MODULE_MANAGER_FWD_DECLARATION(SystemClock)

} // namespace Modules

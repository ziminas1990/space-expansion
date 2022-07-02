#pragma once

#include <Modules/CommonModulesManager.h>
#include <Modules/Ship/Ship.h>

namespace modules {

using ShipsManager = modules::CommonModulesManager<Ship, modules::Cooldown::eShip>;
using ShipsManagerPtr = std::shared_ptr<ShipsManager>;

} // namespace modules

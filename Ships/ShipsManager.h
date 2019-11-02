#pragma once

#include <Modules/CommonModulesManager.h>
#include "Ship.h"

namespace ships {

using ShipsManager = modules::CommonModulesManager<Ship, modules::Cooldown::eShip>;
using ShipsManagerPtr = std::shared_ptr<ShipsManager>;

} // namespace modules

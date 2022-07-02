#pragma once

#include <Modules/CommonModulesManager.h>
#include <Modules/Ship/Ship.h>

namespace modules {

class ShipManager
: public modules::CommonModulesManager<Ship, modules::Cooldown::eShip>
{};

} // namespace modules

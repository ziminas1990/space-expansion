#pragma once

#include <Modules/CommonModulesManager.h>
#include "Ship.h"

namespace ships {

using ShipsManager        = modules::CommonModulesManager<Ship>;
using ShipsManagerPtr     = std::shared_ptr<ShipsManager>;
using ShipsManagerWeakPtr = std::weak_ptr<ShipsManager>;

} // namespace modules

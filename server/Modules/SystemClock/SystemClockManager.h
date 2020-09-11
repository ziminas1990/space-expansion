#pragma once

#include "SystemClock.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using SystemClockManager    = CommonModulesManager<SystemClock, Cooldown::eSystemClock>;
using SystemClockManagerPtr = std::shared_ptr<SystemClockManager>;

} // namespace modules

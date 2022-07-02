#pragma once

#include "SystemClock.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

class SystemClockManager
: public CommonModulesManager<SystemClock, Cooldown::eSystemClock>
{};

} // namespace modules

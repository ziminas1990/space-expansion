#pragma once

#include "Engine.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using EngineManager    = CommonModulesManager<Engine, 100000>;
using EngineManagerPtr = std::shared_ptr<EngineManager>;

} // namespace modules

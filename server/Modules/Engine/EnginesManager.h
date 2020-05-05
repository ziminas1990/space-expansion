#pragma once

#include "Engine.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using EngineManager    = CommonModulesManager<Engine, modules::Cooldown::eEngine>;
using EngineManagerPtr = std::shared_ptr<EngineManager>;

} // namespace modules

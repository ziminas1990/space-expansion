#pragma once

#include "Engine.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

class EngineManager
: public CommonModulesManager<Engine, modules::Cooldown::eEngine>
{};

} // namespace modules

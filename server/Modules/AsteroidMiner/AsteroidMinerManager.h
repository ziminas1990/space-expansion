#pragma once

#include "AsteroidMiner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using AsteroidMinerManager    =
  CommonModulesManager<AsteroidMiner, Cooldown::eAsteroidMiner>;
using AsteroidMinerManagerPtr = std::shared_ptr<AsteroidMinerManager>;

} // namespace modules

#pragma once

#include "AsteroidMiner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using AsteroidMinerManager    =
  CommonModulesManager<AsteroidMiner, Cooldown::eAsteroidScanner>;
using AsteroidMinerManagerPtr = std::shared_ptr<AsteroidMinerManager>;

} // namespace modules

#pragma once

#include "AsteroidScanner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using AsteroidScannerManager    =
  CommonModulesManager<AsteroidScanner, Cooldown::eAsteroidScanner>;
using AsteroidScannerManagerPtr = std::shared_ptr<AsteroidScannerManager>;

} // namespace modules

#pragma once

#include "AsteroidScanner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using AsteroidScannerManager    = CommonModulesManager<AsteroidScanner, 205 * 1000>;
using AsteroidScannerManagerPtr = std::shared_ptr<AsteroidScannerManager>;

} // namespace modules

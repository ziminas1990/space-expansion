#pragma once

#include "CelestialScanner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using CelestialScannerManager =
CommonModulesManager<CelestialScanner, Cooldown::eCelestialScanner>;
using CelestialScannerManagerPtr = std::shared_ptr<CelestialScannerManager>;

} // namespace modules

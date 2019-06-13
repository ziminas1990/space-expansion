#pragma once

#include "CelestialScanner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using CelestialScannerManager    = CommonModulesManager<CelestialScanner, 200 * 1000>;
using CelestialScannerManagerPtr = std::shared_ptr<CelestialScannerManager>;

} // namespace modules

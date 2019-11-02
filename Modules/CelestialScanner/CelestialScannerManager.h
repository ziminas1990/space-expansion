#pragma once

#include "CelestialScanner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using CelestialScannerManager =
CommonModulesManager<CelestialScanner, modules::Cooldown::eCelestialScanner>;
using CelestialScannerManagerPtr = std::shared_ptr<CelestialScannerManager>;

} // namespace modules

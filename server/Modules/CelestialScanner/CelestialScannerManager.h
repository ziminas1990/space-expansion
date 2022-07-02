#pragma once

#include "CelestialScanner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

class CelestialScannerManager 
: public CommonModulesManager<CelestialScanner, Cooldown::eCelestialScanner>
{};

} // namespace modules

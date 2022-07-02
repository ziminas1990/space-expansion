#pragma once

#include "AsteroidScanner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

class AsteroidScannerManager : 
public CommonModulesManager<AsteroidScanner, Cooldown::eAsteroidScanner>
{};

} // namespace modules

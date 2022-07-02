#pragma once

#include "PassiveScanner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

class PassiveScannerManager
: public CommonModulesManager<PassiveScanner, Cooldown::ePassiveScanner>
{};

} // namespace modules

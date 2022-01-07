#pragma once

#include "PassiveScanner.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using PassiveScannerManager =
CommonModulesManager<PassiveScanner, Cooldown::ePassiveScanner>;
using PassiveScannerManagerPtr = std::shared_ptr<PassiveScannerManager>;

} // namespace modules

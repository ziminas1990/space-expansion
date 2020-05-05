#pragma once

#include "BlueprintsStorage.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using BlueprintsStorageManager =
CommonModulesManager<BlueprintsStorage, modules::Cooldown::eBlueprintsStorage>;
using BlueprintsStorageManagerPtr = std::shared_ptr<BlueprintsStorageManager>;

} // namespace modules

#pragma once

#include "BlueprintsStorage.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

class BlueprintsStorageManager : 
public CommonModulesManager<BlueprintsStorage, modules::Cooldown::eBlueprintsStorage>
{};

} // namespace modules

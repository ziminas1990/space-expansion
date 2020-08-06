#pragma once

#include "Commutator.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using CommutatorManager    = CommonModulesManager<Commutator, Cooldown::eCommutator>;
using CommutatorManagerPtr = std::shared_ptr<CommutatorManager>;

} // namespace modules

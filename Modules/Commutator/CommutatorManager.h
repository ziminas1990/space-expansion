#pragma once

#include <atomic>
#include <memory>
#include <vector>
#include "Commutator.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

using CommutatorManager    = CommonModulesManager<Commutator>;
using CommutatorManagerPtr = std::shared_ptr<CommutatorManager>;

} // namespace modules

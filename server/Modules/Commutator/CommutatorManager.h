#pragma once

#include "Commutator.h"
#include <Modules/CommonModulesManager.h>

namespace modules
{

class CommutatorManager 
: public CommonModulesManager<Commutator, Cooldown::eCommutator>
{};

} // namespace modules

#pragma once

#include <Modules/AsteroidMiner/AsteroidMiner.h>
#include <Modules/CommonModulesManager.h>

namespace modules
{

class AsteroidMinerManager:
public CommonModulesManager<AsteroidMiner, Cooldown::eAsteroidMiner> 
{};

} // namespace modules

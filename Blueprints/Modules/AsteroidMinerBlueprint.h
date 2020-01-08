#pragma once

#include "ModuleBlueprint.h"
#include <Modules/AsteroidMiner/AsteroidMiner.h>

namespace modules {

class AsteroidMinerBlueprint : public ModuleBlueprint
{
public:
  AsteroidMinerBlueprint() : m_nMaxDistance(0), m_nCycleTimeMs(0), m_nYieldPerCycle(0)
  {}

  AsteroidMinerBlueprint& setMaxDistance(uint32_t nMaxDistance)
  {
    m_nMaxDistance = nMaxDistance;
    return *this;
  }

  AsteroidMinerBlueprint& setCycleTimeMs(uint32_t nCycleTimeMs)
  {
    m_nCycleTimeMs = nCycleTimeMs;
    return *this;
  }

  AsteroidMinerBlueprint& setYielPerSecond(uint32_t nYieldPerCycle)
  {
    m_nYieldPerCycle = nYieldPerCycle;
    return *this;
  }

  AsteroidMinerBlueprint& setContainer(std::string sConatiner)
  {
    m_sContainerName = std::move(sConatiner);
    return *this;
  }

  BaseModulePtr build() const override
  {
    return std::make_shared<AsteroidMiner>(
          m_nMaxDistance, m_nCycleTimeMs, m_nYieldPerCycle, m_sContainerName);
  }

  ModuleBlueprintPtr wrapToSharedPtr() override
  {
    return std::make_shared<AsteroidMinerBlueprint>(std::move(*this));
  }

private:
  uint32_t    m_nMaxDistance;
  uint32_t    m_nCycleTimeMs;
  uint32_t    m_nYieldPerCycle;
  std::string m_sContainerName;
};

} // namespace modules

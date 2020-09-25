#pragma once

#include <inttypes.h>
#include <Blueprints/BaseBlueprint.h>
#include <Modules/AsteroidMiner/AsteroidMiner.h>
#include <Utils/YamlReader.h>
#include <Utils/YamlDumper.h>

namespace blueprints {

class AsteroidMinerBlueprint : public BaseBlueprint
{
public:
  AsteroidMinerBlueprint() : m_nMaxDistance(0), m_nCycleTimeMs(0), m_nYieldPerCycle(0)
  {}

  modules::BaseModulePtr
  build(std::string sName, world::PlayerWeakPtr pOwner) const override
  {
    return std::make_shared<modules::AsteroidMiner>(
          std::move(sName), std::move(pOwner), m_nMaxDistance, m_nCycleTimeMs,
          m_nYieldPerCycle);
  }

  bool load(YAML::Node const& data) override
  {
    return BaseBlueprint::load(data)
        && utils::YamlReader(data)
           .read("max_distance",    m_nMaxDistance)
           .read("cycle_time_ms",   m_nCycleTimeMs)
           .read("yield_per_cycle", m_nYieldPerCycle);
  }

  void dump(YAML::Node& out) const override
  {
    BaseBlueprint::dump(out);
    utils::YamlDumper(out)
            .add("max_distance",    m_nMaxDistance)
            .add("cycle_time_ms",   m_nCycleTimeMs)
            .add("yield_per_cycle", m_nYieldPerCycle);
  }

private:
  uint32_t m_nMaxDistance;
  uint32_t m_nCycleTimeMs;
  uint32_t m_nYieldPerCycle;
};

} // namespace modules

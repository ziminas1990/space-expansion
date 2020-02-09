#pragma once

#include <inttypes.h>
#include <Blueprints/BaseBlueprint.h>
#include <Modules/AsteroidMiner/AsteroidMiner.h>
#include <Utils/YamlReader.h>
#include <Utils/YamlDumper.h>

namespace modules {

class AsteroidMinerBlueprint : public BaseBlueprint
{
public:
  AsteroidMinerBlueprint() : m_nMaxDistance(0), m_nCycleTimeMs(0), m_nYieldPerCycle(0)
  {}

  BaseModulePtr build(std::string sName, BlueprintsLibrary const&) const override
  {
    return std::make_shared<AsteroidMiner>(
          std::move(sName), m_nMaxDistance, m_nCycleTimeMs, m_nYieldPerCycle,
          m_sContainerName);
  }

  bool load(YAML::Node const& data) override
  {
    return BaseBlueprint::load(data)
        && utils::YamlReader(data)
           .read("max_distance",    m_nMaxDistance)
           .read("cycle_time_ms",   m_nCycleTimeMs)
           .read("yield_per_cycle", m_nYieldPerCycle)
           .read("container",       m_sContainerName);
  }

  void dump(YAML::Node& out) const override
  {
    BaseBlueprint::dump(out);
    utils::YamlDumper(out)
            .add("max_distance",    m_nMaxDistance)
            .add("cycle_time_ms",   m_nCycleTimeMs)
            .add("yield_per_cycle", m_nYieldPerCycle)
            .add("container",       m_sContainerName);
  }

private:
  uint32_t    m_nMaxDistance;
  uint32_t    m_nCycleTimeMs;
  uint32_t    m_nYieldPerCycle;
  std::string m_sContainerName;
};

} // namespace modules

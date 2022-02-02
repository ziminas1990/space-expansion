#pragma once

#include <Blueprints/BaseBlueprint.h>
#include <Modules/PassiveScanner/PassiveScanner.h>
#include <Utils/YamlDumper.h>
#include <Utils/YamlReader.h>

namespace blueprints {

class PassiveScannerBlueprint : public BaseBlueprint
{
public:

  PassiveScannerBlueprint()
    : m_nMaxScanningRadiusKm(0), m_nEdgeUpdateTimeMs(0)
  {}

  modules::BaseModulePtr build(
      std::string sName, world::PlayerWeakPtr pOwner) const override
  {
    return std::make_shared<modules::PassiveScanner>(
          std::move(sName), std::move(pOwner),
          m_nMaxScanningRadiusKm, m_nEdgeUpdateTimeMs);
  }

  bool load(YAML::Node const& data) override
  {
    return BaseBlueprint::load(data)
        && utils::YamlReader(data)
           .read("max_scanning_radius_km", m_nMaxScanningRadiusKm)
           .read("edge_update_time_ms",    m_nEdgeUpdateTimeMs);
  }

  void dump(YAML::Node& out) const override
  {
    BaseBlueprint::dump(out);
    utils::YamlDumper(out)
            .add("max_scanning_radius_km", m_nMaxScanningRadiusKm)
            .add("edge_update_time_ms",    m_nEdgeUpdateTimeMs);
  }

private:
  uint32_t m_nMaxScanningRadiusKm;
  uint32_t m_nEdgeUpdateTimeMs;
};

} // namespace modules

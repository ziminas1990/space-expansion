#pragma once

#include <Blueprints/BaseBlueprint.h>
#include <Modules/CelestialScanner/CelestialScanner.h>
#include <Utils/YamlDumper.h>
#include <Utils/YamlReader.h>

namespace blueprints {

class CelestialScannerBlueprint : public BaseBlueprint
{
public:

  CelestialScannerBlueprint()
    : m_nMaxScanningRadiusKm(0), m_nProcessingTimeUs(0)
  {}

  modules::BaseModulePtr build(std::string sName, world::PlayerWeakPtr pOwner) const override
  {
    return std::make_shared<modules::CelestialScanner>(
          std::move(sName), std::move(pOwner),
          m_nMaxScanningRadiusKm, m_nProcessingTimeUs);
  }

  bool load(YAML::Node const& data) override
  {
    return BaseBlueprint::load(data)
        && utils::YamlReader(data)
           .read("max_scanning_radius_km", m_nMaxScanningRadiusKm)
           .read("processing_time_us",     m_nProcessingTimeUs);
  }

  void dump(YAML::Node& out) const override
  {
    BaseBlueprint::dump(out);
    utils::YamlDumper(out)
            .add("max_scanning_radius_km", m_nMaxScanningRadiusKm)
            .add("processing_time_us",     m_nProcessingTimeUs);
  }

private:
  uint32_t m_nMaxScanningRadiusKm;
  uint32_t m_nProcessingTimeUs;
};

} // namespace modules

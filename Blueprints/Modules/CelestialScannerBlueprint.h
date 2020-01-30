#pragma once

#include <Blueprints/Modules/ModuleBlueprint.h>
#include <Modules/CelestialScanner/CelestialScanner.h>
#include <Utils/YamlDumper.h>
#include <Utils/YamlReader.h>

namespace modules {

class CelestialScannerBlueprint : public ModuleBlueprint
{
public:

  CelestialScannerBlueprint()
    : m_nMaxScanningRadiusKm(0), m_nProcessingTimeUs(0)
  {}

  BaseModulePtr build() const override
  {
    return std::make_shared<CelestialScanner>(
          m_nMaxScanningRadiusKm, m_nProcessingTimeUs);
  }

  bool load(YAML::Node const& data) override
  {
    return utils::YamlReader(data)
        .read("max_scanning_radius_km", m_nMaxScanningRadiusKm)
        .read("processing_time_us",     m_nProcessingTimeUs);
  }

  void dump(YAML::Node& out) const override
  {
    utils::YamlDumper(out)
            .add("max_scanning_radius_km", m_nMaxScanningRadiusKm)
            .add("processing_time_us",     m_nProcessingTimeUs);
  }

private:
  uint32_t m_nMaxScanningRadiusKm;
  uint32_t m_nProcessingTimeUs;
};

} // namespace modules

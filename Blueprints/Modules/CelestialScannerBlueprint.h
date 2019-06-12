#pragma once

#include <Blueprints/Modules/ModuleBlueprint.h>
#include <Modules/CelestialScanner/CelestialScanner.h>

namespace modules {

class CelestialScannerBlueprint : public ModuleBlueprint
{
public:

  CelestialScannerBlueprint()
    : m_nMaxScanningRadiusKm(0), m_nProcessingTimeUs(0)
  {}

  CelestialScannerBlueprint& setMaxScanningRadiusKm(uint32_t nMaxScanningRadiusKm)
  {
    m_nMaxScanningRadiusKm = nMaxScanningRadiusKm;
    return *this;
  }

  CelestialScannerBlueprint& setProcessingTimeUs(uint32_t nProcessingTimeUs)
  {
    m_nProcessingTimeUs = nProcessingTimeUs;
    return *this;
  }

  BaseModulePtr build() const override
  {
    return std::make_shared<CelestialScanner>(
          m_nMaxScanningRadiusKm, m_nProcessingTimeUs);
  }

  ModuleBlueprintPtr wrapToSharedPtr() override
  {
    return std::make_shared<CelestialScannerBlueprint>(std::move(*this));
  }

private:
  uint32_t m_nMaxScanningRadiusKm;
  uint32_t m_nProcessingTimeUs;
};

} // namespace modules

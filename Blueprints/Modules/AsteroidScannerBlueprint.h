#pragma once

#include <Blueprints/Modules/ModuleBlueprint.h>
#include <Modules/AsteroidScanner/AsteroidScanner.h>

namespace modules {

class AsteroidScannerBlueprint : public ModuleBlueprint
{
public:

  AsteroidScannerBlueprint()
    : m_nMaxScanningDistance(0), m_nScanningTimeMs(0)
  {}

  AsteroidScannerBlueprint& setMaxScanningRadiusKm(uint32_t nMaxScanningRadiusKm)
  {
    m_nMaxScanningDistance = nMaxScanningRadiusKm;
    return *this;
  }

  AsteroidScannerBlueprint& setScanningTimeMs(uint32_t nScanningTimeMs)
  {
    m_nScanningTimeMs = nScanningTimeMs;
    return *this;
  }

  BaseModulePtr build() const override
  {
    return std::make_shared<AsteroidScanner>(m_nMaxScanningDistance, m_nScanningTimeMs);
  }

  ModuleBlueprintPtr wrapToSharedPtr() override
  {
    return std::make_shared<AsteroidScannerBlueprint>(std::move(*this));
  }

private:
  uint32_t m_nMaxScanningDistance;
  uint32_t m_nScanningTimeMs;
};

} // namespace modules

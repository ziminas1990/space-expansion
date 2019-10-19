#include "BlueprintFactory.h"

#include "EngineBlueprint.h"
#include "CelestialScannerBlueprint.h"
#include "AsteroidScannerBlueprint.h"

#include <Utils/YamlReader.h>

namespace modules
{

ModuleBlueprintPtr BlueprintsFactory::make(YAML::Node const &data)
{
  utils::YamlReader reader(data);
  std::string sModuleType;
  if (!reader.read("type", sModuleType)) {
    assert(false);
    return ModuleBlueprintPtr();
  }

  if (sModuleType == "engine") {
    uint32_t maxThrust = 0;
    if (!reader.read("maxThrust", maxThrust)) {
      assert(false);
      return ModuleBlueprintPtr();;
    }
    return EngineBlueprint().setMaxThrust(maxThrust).wrapToSharedPtr();

  } else if (sModuleType == "CelestialScanner") {
    uint32_t nMaxScanningRadiusKm = 0;
    uint32_t nProcessingTimeUs    = 0;
    if (!reader.read("max_scanning_radius_km", nMaxScanningRadiusKm)
               .read("processing_time_us",     nProcessingTimeUs))
    {
      assert(false);
      return ModuleBlueprintPtr();
    }
    return CelestialScannerBlueprint()
           .setMaxScanningRadiusKm(nMaxScanningRadiusKm)
           .setProcessingTimeUs(nProcessingTimeUs)
           .wrapToSharedPtr();
  } else if (sModuleType == "AsteroidScanner") {
    uint32_t nMaxScanningDistance = 0;
    uint32_t nScanningTimeMs      = 0;
    if (!reader.read("max_scanning_distance", nMaxScanningDistance)
               .read("scanning_time_ms",      nScanningTimeMs))
    {
      assert(false);
      return AsteroidScannerBlueprint().wrapToSharedPtr();
    }
    return AsteroidScannerBlueprint()
        .setMaxScanningRadiusKm(nMaxScanningDistance)
        .setScanningTimeMs(nScanningTimeMs)
        .wrapToSharedPtr();
  }

  assert(false);
  return ModuleBlueprintPtr();
}

} // namespace modules

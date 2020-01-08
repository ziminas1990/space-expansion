#include "BlueprintFactory.h"

#include "EngineBlueprint.h"
#include "CelestialScannerBlueprint.h"
#include "AsteroidScannerBlueprint.h"
#include "ResourceContainerBlueprint.h"
#include "AsteroidMinerBlueprint.h"

#include <Utils/YamlReader.h>

namespace modules
{

ModuleBlueprintPtr BlueprintsFactory::make(YAML::Node const &data)
{
  utils::YamlReader reader(data);
  std::string sModuleClass;
  if (!reader.read("type", sModuleClass)) {
    assert(false);
    return ModuleBlueprintPtr();
  }

  if (sModuleClass == "engine") {
    uint32_t maxThrust = 0;
    if (!reader.read("maxThrust", maxThrust)) {
      assert(false);
      return ModuleBlueprintPtr();
    }
    return EngineBlueprint().setMaxThrust(maxThrust).wrapToSharedPtr();

  } else if (sModuleClass == "CelestialScanner") {
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

  } else if (sModuleClass == "AsteroidScanner") {
    uint32_t nMaxScanningDistance = 0;
    uint32_t nScanningTimeMs      = 0;
    bool lIsOk = reader.read("max_scanning_distance", nMaxScanningDistance)
                       .read("scanning_time_ms",      nScanningTimeMs);
    assert(lIsOk);
    if (lIsOk)
    {
      return AsteroidScannerBlueprint()
          .setMaxScanningRadiusKm(nMaxScanningDistance)
          .setScanningTimeMs(nScanningTimeMs)
          .wrapToSharedPtr();
    }

  } else if (sModuleClass == "ResourceContainer") {
    uint32_t nVolume = 0;
    bool lIsOk = reader.read("volume", nVolume);
    assert(lIsOk);
    if (lIsOk)
    {
      return ResourceContainerBlueprint()
          .setVolume(nVolume)
          .wrapToSharedPtr();
    }

  } else if (sModuleClass == "AsteroidMiner") {
    uint32_t    nDistance      = 0;
    uint32_t    nCycleTimeMs   = 0;
    uint32_t    nYieldPerCycle = 0;
    std::string sContainerName;
    bool lIsOk = reader.read("max_distance",    nDistance)
                       .read("cycle_time_ms",   nCycleTimeMs)
                       .read("yield_per_cycle", nYieldPerCycle)
                       .read("container",       sContainerName);
    assert(lIsOk);
    if (lIsOk) {
      return AsteroidMinerBlueprint()
          .setMaxDistance(nDistance)
          .setCycleTimeMs(nCycleTimeMs)
          .setYielPerSecond(nYieldPerCycle)
          .setContainer(sContainerName)
          .wrapToSharedPtr();
    }
  }

  assert(false);
  return ModuleBlueprintPtr();
}

} // namespace modules

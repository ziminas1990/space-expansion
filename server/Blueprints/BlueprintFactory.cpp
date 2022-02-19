#include <Blueprints/BlueprintFactory.h>

#include <Blueprints/Modules/EngineBlueprint.h>
#include <Blueprints/Modules/CelestialScannerBlueprint.h>
#include <Blueprints/Modules/PassiveScannerBlueprint.h>
#include <Blueprints/Modules/AsteroidScannerBlueprint.h>
#include <Blueprints/Modules/ResourceContainerBlueprint.h>
#include <Blueprints/Modules/AsteroidMinerBlueprint.h>
#include <Blueprints/Modules/Shipyard.h>

#include <Utils/YamlReader.h>

namespace blueprints
{

BaseBlueprintPtr BlueprintsFactory::make(std::string const& sModuleType,
                                         YAML::Node const& data)
{
  BaseBlueprintPtr pBlueprint;
  if (sModuleType == "Engine") {
    pBlueprint = std::make_shared<EngineBlueprint>();
  } else if (sModuleType == "CelestialScanner") {
    pBlueprint = std::make_shared<CelestialScannerBlueprint>();
  } else if (sModuleType == "PassiveScanner") {
    pBlueprint = std::make_shared<PassiveScannerBlueprint>();
  } else if (sModuleType == "AsteroidScanner") {
    pBlueprint = std::make_shared<AsteroidScannerBlueprint>();
  } else if (sModuleType == "ResourceContainer") {
    pBlueprint = std::make_shared<ResourceContainerBlueprint>();
  } else if (sModuleType == "AsteroidMiner") {
    pBlueprint = std::make_shared<AsteroidMinerBlueprint>();
  } else if (sModuleType == "Shipyard") {
    pBlueprint = std::make_shared<ShipyardBlueprint>();
  }

  assert(pBlueprint != nullptr);
  if (!pBlueprint || !pBlueprint->load(data)) {
    assert(nullptr == "Failed to read blueprint");
    return BaseBlueprintPtr();
  }
  return pBlueprint;
}

} // namespace modules

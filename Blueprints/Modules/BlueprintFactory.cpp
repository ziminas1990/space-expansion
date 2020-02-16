#include <Blueprints/BlueprintFactory.h>

#include "EngineBlueprint.h"
#include "CelestialScannerBlueprint.h"
#include "AsteroidScannerBlueprint.h"
#include "ResourceContainerBlueprint.h"
#include "AsteroidMinerBlueprint.h"
#include "Shipyard.h"

#include <Utils/YamlReader.h>

namespace blueprints
{

BaseBlueprintPtr BlueprintsFactory::make(std::string const& sModuleType,
                                         YAML::Node const& data)
{
  utils::YamlReader reader(data);

  BaseBlueprintPtr pBlueprint;

  if (sModuleType == "Engine") {
    pBlueprint = std::make_shared<EngineBlueprint>();
  } else if (sModuleType == "CelestialScanner") {
    pBlueprint = std::make_shared<CelestialScannerBlueprint>();
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

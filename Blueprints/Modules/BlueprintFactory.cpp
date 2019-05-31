#include "BlueprintFactory.h"

#include "Engine/EngineBlueprint.h"

#include <Utils/YamlReader.h>

namespace modules
{

ModuleBlueprintPtr BlueprintsFactory::make(YAML::Node const &data)
{
  utils::YamlReader reader(data);
  std::string sModuleType;
  if (!reader.read("type", sModuleType))
    return ModuleBlueprintPtr();

  if (sModuleType == "engine") {
    uint32_t maxThrust = 0;
    return reader.read("maxThrust", maxThrust)
        ? EngineBlueprint().setMaxThrust(maxThrust).wrapToSharedPtr()
        : ModuleBlueprintPtr();
  }

  return ModuleBlueprintPtr();
}

} // namespace modules

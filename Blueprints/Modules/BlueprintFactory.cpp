#include "BlueprintFactory.h"

#include "Engine/EngineBlueprint.h"

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
    if(!reader.read("maxThrust", maxThrust)) {
      assert(false);
      return ModuleBlueprintPtr();;
    }
    return EngineBlueprint().setMaxThrust(maxThrust).wrapToSharedPtr();
  }

  assert(false);
  return ModuleBlueprintPtr();
}

} // namespace modules

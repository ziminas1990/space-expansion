#pragma once

#include "ModuleBlueprint.h"
#include <Utils/YamlForwardDeclarations.h>

namespace modules
{

class BlueprintsFactory
{
public:
  static ModuleBlueprintPtr make(YAML::Node const& data);
};

} // namespace modules

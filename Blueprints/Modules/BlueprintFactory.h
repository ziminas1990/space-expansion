#pragma once

#include "ModuleBlueprint.h"
#include <Utils/YamlForwardDeclarations.h>

namespace modules
{

class BlueprintsFactory
{
public:
  static ModuleBlueprintPtr make(std::string const& sModuleType, YAML::Node const& data);
};

} // namespace modules

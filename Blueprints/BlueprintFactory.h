#pragma once

#include <Blueprints/AbstractBlueprint.h>
#include <Utils/YamlForwardDeclarations.h>

namespace modules
{

class BlueprintsFactory
{
public:
  static AbstractBlueprintPtr make(std::string const& sModuleType, YAML::Node const& data);
};

} // namespace modules

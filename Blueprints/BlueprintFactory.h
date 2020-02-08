#pragma once

#include <Blueprints/BaseBlueprint.h>
#include <Utils/YamlForwardDeclarations.h>

namespace modules
{

class BlueprintsFactory
{
public:
  static BaseBlueprintPtr make(std::string const& sModuleType, YAML::Node const& data);
};

} // namespace modules

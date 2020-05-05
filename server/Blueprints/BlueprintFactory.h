#pragma once

#include <Blueprints/BaseBlueprint.h>
#include <Utils/YamlForwardDeclarations.h>

namespace blueprints
{

class BlueprintsFactory
{
public:
  static BaseBlueprintPtr make(std::string const& sModuleType, YAML::Node const& data);
};

} // namespace modules

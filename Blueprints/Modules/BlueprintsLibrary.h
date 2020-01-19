#pragma once

#include <map>

#include <Utils/YamlForwardDeclarations.h>
#include "BlueprintName.h"
#include "ModuleBlueprint.h"

namespace modules {

class BlueprintsLibrary
{
  using ModuleTypesMap   = std::map<std::string, ModuleBlueprintPtr>;
  using ModuleClassesMap = std::map<std::string, ModuleTypesMap>;
public:

  bool loadBlueprints(YAML::Node const& modulesSection);

  ModuleBlueprintPtr getBlueprint(BlueprintName const& name) const;

private:
  std::map<BlueprintName, ModuleBlueprintPtr> m_blueprints;
};

} // namespace modules

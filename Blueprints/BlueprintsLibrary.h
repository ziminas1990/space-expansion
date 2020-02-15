#pragma once

#include <map>
#include <memory>
#include <functional>

#include <Utils/YamlForwardDeclarations.h>
#include "BlueprintsForwardDecls.h"
#include "BlueprintName.h"

namespace blueprints {

class BlueprintsLibrary
{
  using ModuleTypesMap   = std::map<std::string, BaseBlueprintPtr>;
  using ModuleClassesMap = std::map<std::string, ModuleTypesMap>;
public:

  bool loadModulesBlueprints(YAML::Node const& modulesSection);
  bool loadShipsBlueprints(YAML::Node const& shipsSection);

  void overwriteBlueprint(BlueprintName const& name, BaseBlueprintPtr pBlueprint);
  BaseBlueprintPtr getBlueprint(BlueprintName const& name) const;
  bool             hasBlueprint(BlueprintName const& name) const;

  void iterate(std::function<bool(const BlueprintName &)> const& viewer) const;

private:
  std::map<BlueprintName, BaseBlueprintPtr> m_blueprints;
};

} // namespace modules

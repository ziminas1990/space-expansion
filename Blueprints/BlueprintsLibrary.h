#pragma once

#include <map>
#include <memory>
#include <functional>

#include <Utils/YamlForwardDeclarations.h>
#include "BlueprintName.h"

namespace modules {

class AbstractBlueprint;
using AbstractBlueprintPtr = std::shared_ptr<AbstractBlueprint>;

class BlueprintsLibrary
{
  using ModuleTypesMap   = std::map<std::string, AbstractBlueprintPtr>;
  using ModuleClassesMap = std::map<std::string, ModuleTypesMap>;
public:

  bool loadModulesBlueprints(YAML::Node const& modulesSection);
  bool loadShipsBlueprints(YAML::Node const& shipsSection);

  AbstractBlueprintPtr getBlueprint(BlueprintName const& name) const;

  void iterate(std::function<bool(const BlueprintName &)> const& viewer) const;

private:
  std::map<BlueprintName, AbstractBlueprintPtr> m_blueprints;
};

} // namespace modules

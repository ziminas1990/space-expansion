#pragma once

#include <memory>
#include <map>
#include <Ships/Ship.h>
#include <Blueprints/BaseBlueprint.h>
#include <Blueprints/BlueprintName.h>
#include <Blueprints/BlueprintsLibrary.h>

#include <Utils/YamlForwardDeclarations.h>

namespace ships {

class ShipBlueprint;
using ShipBlueprintPtr      = std::shared_ptr<ShipBlueprint>;
using ShipBlueprintConstPtr = std::shared_ptr<ShipBlueprint const>;

class ShipBlueprint : public modules::BaseBlueprint
{
public:
  ShipBlueprint(std::string sShipProjectName);

  modules::BaseModulePtr build(std::string sName,
                               modules::BlueprintsLibrary const& library) const override;
  bool load(YAML::Node const& data) override;
  void dump(YAML::Node& out) const override;

private:
  std::string m_sType  = "unknown";
  double      m_weight = 1000;
  double      m_radius = 1;

  std::map<std::string, modules::BlueprintName> m_modules;
};

} // namespace ships

#pragma once

#include <memory>
#include <map>
#include <Ships/Ship.h>
#include <Blueprints/BaseBlueprint.h>
#include <Blueprints/BlueprintName.h>

#include <Utils/YamlForwardDeclarations.h>

namespace blueprints {

class ShipBlueprint;
using ShipBlueprintPtr      = std::shared_ptr<ShipBlueprint>;
using ShipBlueprintConstPtr = std::shared_ptr<ShipBlueprint const>;

class ShipBlueprint : public BaseBlueprint
{
public:
  ShipBlueprint(std::string sShipProjectName);

  modules::BaseModulePtr build(std::string sName,
                               world::PlayerWeakPtr pOwner) const override;
  bool load(YAML::Node const& data) override;
  void dump(YAML::Node& out) const override;

private:
  std::string m_sType  = "unknown";
  double      m_weight = 1000;
  double      m_radius = 1;

  std::map<std::string, BlueprintName> m_modules;
};

} // namespace ships

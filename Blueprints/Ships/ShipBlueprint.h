#pragma once

#include <memory>
#include <map>
#include <Ships/Ship.h>
#include <Blueprints/Modules/ModuleBlueprint.h>

#include <Utils/YamlForwardDeclarations.h>

namespace ships {

class ShipBlueprint;
using ShipBlueprintPtr      = std::shared_ptr<ShipBlueprint>;
using ShipBlueprintConstPtr = std::shared_ptr<ShipBlueprint const>;

class ShipBlueprint
{
public:
  static ShipBlueprintPtr make(YAML::Node const& data);

  virtual ~ShipBlueprint() = default;

  virtual ShipPtr build() const;

  virtual ShipBlueprintPtr wrapToSharedPtr()
  {
    return std::make_shared<ShipBlueprint>(std::move(*this));
  }

  ShipBlueprint& setWeight(double weight);
  ShipBlueprint& setShipType(std::string sShipType);
  ShipBlueprint& addModule(std::string sModuleName,
                           modules::ModuleBlueprintPtr pModuleBlueprint);

private:
  std::string m_sShipType  = "unknown";
  double      m_shipWeight = 1000;

  std::map<std::string, modules::ModuleBlueprintPtr> m_modules;
};


class BlueprintsStore
{
public:
  static ShipBlueprintPtr makeCommandCenterBlueprint();
  static ShipBlueprintPtr makeCorvetBlueprint();
  static ShipBlueprintPtr makeMinerBlueprint();
  static ShipBlueprintPtr makeZondBlueprint();
};

} // namespace ships

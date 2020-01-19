#pragma once

#include <memory>
#include <map>
#include <Ships/Ship.h>
#include <Blueprints/Modules/BlueprintName.h>
#include <Blueprints/Modules/BlueprintsLibrary.h>

#include <Utils/YamlForwardDeclarations.h>

namespace ships {

class ShipBlueprint;
using ShipBlueprintPtr      = std::shared_ptr<ShipBlueprint>;
using ShipBlueprintConstPtr = std::shared_ptr<ShipBlueprint const>;

class ShipBlueprint
{
public:
  static ShipBlueprintPtr make(std::string sShipType, YAML::Node const& data);

  virtual ~ShipBlueprint() = default;

  virtual ShipPtr build(std::string shipName,
                        modules::BlueprintsLibrary const& modules) const;

  virtual ShipBlueprintPtr wrapToSharedPtr()
  {
    return std::make_shared<ShipBlueprint>(std::move(*this));
  }

  ShipBlueprint& setWeightAndRadius(double weight, double radius);
  ShipBlueprint& setShipType(std::string sShipType);
  ShipBlueprint& addModule(std::string sModuleName,
                           modules::BlueprintName sBlueprintName);

private:
  std::string m_sType  = "unknown";
  double      m_weight = 1000;
  double      m_radius = 1;

  std::map<std::string, modules::BlueprintName> m_modules;
};

} // namespace ships

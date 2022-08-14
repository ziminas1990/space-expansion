#pragma once

#include <memory>
#include <map>
#include <string>
#include <Blueprints/BaseBlueprint.h>
#include <Blueprints/BlueprintName.h>
#include <Blueprints/BlueprintsLibrary.h>

#include <Utils/YamlForwardDeclarations.h>

namespace world {
class Player;
using PlayerWeakPtr = std::weak_ptr<Player>;
}

namespace modules {
class Ship;
using ShipPtr     = std::shared_ptr<Ship>;
using ShipWeakPtr = std::weak_ptr<Ship>;
}

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
    // Use this blueprint to build a new ship with the specified 'sName' name
    // for the specified 'pOwner' player.

  modules::ShipPtr build(std::string sName,
                         world::PlayerWeakPtr pOwner,
                         const BlueprintsLibrary& blueprintsLibrary) const;
    // Use this blueprint to build a new ship with the specified 'sName' name
    // for the specified 'pOwner' player. Use the specified 'blueprintsLibrary'
    // to fetch blueprints for ship's modules.

  bool checkDependencies(blueprints::BlueprintsLibrary const& library) const;
    // Check, if all required module's blueprints are presented in the specified
    // 'library'. Return false if ship can't be built because at least one module's
    // blueprint doesn't exist in the 'library'.

  bool exportTotalExpenses(blueprints::BlueprintsLibrary const& library,
                           world::ResourcesArray& total) const;
    // Calculate total expenses for ships producing. In includes expenses for the ship
    // hull and, additionally, expenses for producing all modules, that are required
    // for the ship. The specified 'library' will be used to get modules blueprints.
    // Return false, if some of the ship's modules bluperint is not found in the
    // 'library'. Otherwise return true.

  bool load(YAML::Node const& data) override;
  void dump(YAML::Node& out) const override;


private:
  std::string m_sType  = "unknown";
  double      m_weight = 1000;
  double      m_radius = 1;

  std::map<std::string, BlueprintName> m_modules;
};

} // namespace modules

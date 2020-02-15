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

namespace ships {
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

  ships::ShipPtr build(std::string sName,
                       world::PlayerWeakPtr pOwner,
                       blueprints::BlueprintsLibrary const& customLibrary) const;
    // Overrided build() function uses player's blueprints library to create ship's
    // modules, but this signature uses 'customLibrary' instead.

  bool checkDependencies(blueprints::BlueprintsLibrary const& library) const;
    // Check, if all required module's blueprints are presented in the specified
    // 'library'. Return false if ship can't be built because at least one module's
    // blueprint doesn't exist in the 'library'.

  bool load(YAML::Node const& data) override;
  void dump(YAML::Node& out) const override;


private:
  std::string m_sType  = "unknown";
  double      m_weight = 1000;
  double      m_radius = 1;

  std::map<std::string, BlueprintName> m_modules;
};

} // namespace ships

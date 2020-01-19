#include "ShipBlueprintsLibrary.h"
#include "ShipBlueprint.h"
#include <yaml-cpp/yaml.h>

namespace ships {

bool ShipBlueprintsLibrary::loadBlueprints(YAML::Node const& shipsSection)
{
  for(auto const& shipInfo : shipsSection) {
    std::string const& sShipClassName = shipInfo.first.as<std::string>();

    ShipBlueprintPtr pBlueprint = ShipBlueprint::make(sShipClassName, shipInfo.second);
    assert(pBlueprint != nullptr);
    if (!pBlueprint) {
      return false;
    }

    m_blueprints[sShipClassName] = pBlueprint;
  }
  return true;
}

ShipBlueprintPtr ShipBlueprintsLibrary::getBlueprint(std::string const& name) const
{
  auto I = m_blueprints.find(name);
  return I != m_blueprints.end() ? I->second : ShipBlueprintPtr();
}



} // namespace ships

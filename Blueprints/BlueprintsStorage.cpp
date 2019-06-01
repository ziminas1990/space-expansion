#include "BlueprintsStorage.h"

#include <yaml-cpp/yaml.h>

namespace blueprints {

bool BlueprintsStorage::loadBlueprints(YAML::Node const& data)
{
  for (auto const kv : data)
  {
    std::string sBlueprintName = kv.first.as<std::string>();
    ships::ShipBlueprintPtr pBlueprint = ships::ShipBlueprint::make(kv.second);
    pBlueprint->setShipType(sBlueprintName);
    assert(pBlueprint);
    if (!pBlueprint)
      return false;
    m_blueprints.insert(std::make_pair(std::move(sBlueprintName), std::move(pBlueprint)));
  }
  return true;
}

ships::ShipBlueprintConstPtr
BlueprintsStorage::getBlueprint(std::string const& sShipType) const
{
  auto I = m_blueprints.find(sShipType);
  return I != m_blueprints.end() ? I->second : ships::ShipBlueprintConstPtr();
}

} // namespace blueprints

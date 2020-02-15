#include "BlueprintsLibrary.h"

#include "BlueprintFactory.h"
#include "Ships/ShipBlueprint.h"
#include <yaml-cpp/yaml.h>

namespace blueprints {

bool BlueprintsLibrary::loadModulesBlueprints(YAML::Node const& modulesSection)
{
  for(auto const& modulesClassInfo : modulesSection) {
    std::string const& sModuleClass = modulesClassInfo.first.as<std::string>();

    for (auto const& moduleTypeInfo : modulesClassInfo.second) {
      std::string const& sModuleType = moduleTypeInfo.first.as<std::string>();

      BaseBlueprintPtr pBlueprint =
          BlueprintsFactory::make(sModuleClass, moduleTypeInfo.second);
      assert(pBlueprint != nullptr);
      if (!pBlueprint) {
        return false;
      }

      m_blueprints[BlueprintName(sModuleClass, sModuleType)] = pBlueprint;
    }
  }
  return true;
}

bool BlueprintsLibrary::loadShipsBlueprints(YAML::Node const& shipsSection)
{
  for(auto const& shipInfo : shipsSection) {
      std::string const& sShipClassName = shipInfo.first.as<std::string>();

      ShipBlueprintPtr pBlueprint = std::make_shared<ShipBlueprint>(sShipClassName);
      if (!pBlueprint->load(shipInfo.second)) {
        assert(false);
        return false;
      }

      m_blueprints[BlueprintName("Ship", sShipClassName)] = pBlueprint;
    }
  return true;
}

void BlueprintsLibrary::overwriteBlueprint(
    BlueprintName const& name, BaseBlueprintPtr pBlueprint)
{
  m_blueprints[name] = std::move(pBlueprint);
}

BaseBlueprintPtr BlueprintsLibrary::getBlueprint(BlueprintName const& name) const
{
  auto I = m_blueprints.find(name);
  return I != m_blueprints.end() ? I->second : BaseBlueprintPtr();
}

bool BlueprintsLibrary::hasBlueprint(const BlueprintName &name) const
{
  auto I = m_blueprints.find(name);
  return I != m_blueprints.end();
}

void BlueprintsLibrary::iterate(std::function<bool(BlueprintName const&)> const& viewer) const
{
  for(auto const& kv : m_blueprints) {
    if (!viewer(kv.first))
      return;
  }
}

} // namespace modules

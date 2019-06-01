#include "ShipBlueprint.h"

#include <yaml-cpp/yaml.h>
#include <Utils/YamlReader.h>
#include <Blueprints/Modules/Engine/EngineBlueprint.h>
#include <Blueprints/Modules/BlueprintFactory.h>

namespace ships {

ShipBlueprintPtr ShipBlueprint::make(YAML::Node const& data)
{
  std::string sShipType;
  double      shipWeight;

  if (!utils::YamlReader(data).read("weight", shipWeight)) {
    assert(false);
    return ShipBlueprintPtr();
  }

  ShipBlueprintPtr pBlueprint = std::make_shared<ShipBlueprint>();
  pBlueprint->setShipType(sShipType);
  pBlueprint->setWeight(shipWeight);

  for (auto const& kv : data["modules"]) {
    std::string sModuleName = kv.first.as<std::string>();
    if (pBlueprint->m_modules.find(sModuleName) != pBlueprint->m_modules.end()) {
      // Duplicate detected
      assert(false);
      return ShipBlueprintPtr();
    }
    modules::ModuleBlueprintPtr pModuleBlueprint =
        modules::BlueprintsFactory::make(kv.second);
    assert(pModuleBlueprint);
    if (!pModuleBlueprint)
      return ShipBlueprintPtr();
    pBlueprint->addModule(std::move(sModuleName), std::move(pModuleBlueprint));
  }
  return pBlueprint;
}

ShipPtr ShipBlueprint::build() const
{
  ShipPtr pShip = std::make_shared<Ship>(m_sShipType, m_shipWeight);
  for (auto const& kv : m_modules)
  {
    modules::BaseModulePtr pModule = kv.second->build();
    pShip->installModule(kv.first, pModule);
  }
  return pShip;
}

ShipBlueprint& ShipBlueprint::setWeight(double weight)
{
  m_shipWeight = weight;
  return *this;
}

ShipBlueprint& ShipBlueprint::setShipType(std::string sShipType)
{
  m_sShipType = std::move(sShipType);
  return *this;
}

ShipBlueprint& ShipBlueprint::addModule(
    std::string sModuleName, modules::ModuleBlueprintPtr pModuleBlueprint)
{
  if (m_modules.find(sModuleName) != m_modules.end())
    return *this;
  m_modules.insert(std::make_pair(std::move(sModuleName), std::move(pModuleBlueprint)));
  return *this;
}

} // namespace ships

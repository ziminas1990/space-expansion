#include "ShipBlueprint.h"

#include <yaml-cpp/yaml.h>
#include <Utils/YamlReader.h>
#include <Utils/StringUtils.h>
#include <Blueprints/Modules/EngineBlueprint.h>

namespace ships {

ShipBlueprintPtr ShipBlueprint::make(std::string sShipType, YAML::Node const& data)
{
  double      shipWeight;
  double      shipRadius;

  if (!utils::YamlReader(data)
      .read("weight", shipWeight)
      .read("radius", shipRadius)) {
    assert(false);
    return ShipBlueprintPtr();
  }

  ShipBlueprintPtr pBlueprint = std::make_shared<ShipBlueprint>();
  pBlueprint->setShipType(std::move(sShipType));
  pBlueprint->setWeightAndRadius(shipWeight, shipRadius);

  for (auto const& kv : data["modules"]) {
    std::string sModuleName = kv.first.as<std::string>();
    if (pBlueprint->m_modules.find(sModuleName) != pBlueprint->m_modules.end()) {
      // Duplicate detected
      assert(false);
      return ShipBlueprintPtr();
    }

    std::string const& sBlueprintName = kv.second.as<std::string>();
    std::string sModuleClass;
    std::string sModuleType;
    utils::StringUtils::split('/', sBlueprintName, sModuleClass, sModuleType);
    assert(!sModuleClass.empty() && !sModuleType.empty());
    pBlueprint->m_modules.insert(
          std::make_pair(
            std::move(sModuleName),
            modules::BlueprintName(std::move(sModuleClass), std::move(sModuleType))));
  }
  return pBlueprint;
}

ShipPtr ShipBlueprint::build(std::string shipName,
                             modules::BlueprintsLibrary const& modules) const
{
  ShipPtr pShip = std::make_shared<Ship>(m_sType, std::move(shipName),
                                         m_weight, m_radius);
  for (auto const& kv : m_modules)
  {
    modules::ModuleBlueprintPtr pBlueprint = modules.getBlueprint(kv.second);
    assert(pBlueprint);
    if (!pBlueprint) {
      return ShipPtr();
    }

    modules::BaseModulePtr pModule = pBlueprint->build();
    pModule->changeModuleName(kv.first);
    pShip->installModule(pModule);
  }
  return pShip;
}

ShipBlueprint& ShipBlueprint::setWeightAndRadius(double weight, double radius)
{
  m_weight = weight;
  m_radius = radius;
  return *this;
}

ShipBlueprint& ShipBlueprint::setShipType(std::string sShipType)
{
  m_sType = std::move(sShipType);
  return *this;
}

ShipBlueprint& ShipBlueprint::addModule(
    std::string sModuleName, modules::BlueprintName sBlueprintName)
{
  if (m_modules.find(sModuleName) != m_modules.end())
    return *this;
  m_modules.insert(std::make_pair(std::move(sModuleName), std::move(sBlueprintName)));
  return *this;
}

} // namespace ships

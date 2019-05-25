#include "ShipBlueprint.h"

#include <Modules/Engine/EngineBlueprint.h>

namespace ships {

ShipPtr ShipBlueprint::build() const
{
  ShipPtr pShip = std::make_shared<Ship>(m_sShipType, m_shipWeight);
  for (modules::ModuleBlueprintPtr const& pModuleBlueprint : m_modules)
  {
    modules::BaseModulePtr pModule = pModuleBlueprint->build();
    pShip->installModule(pModule);
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

ShipBlueprint& ShipBlueprint::addModule(modules::ModuleBlueprintPtr pModuleBlueprint)
{
  m_modules.push_back(pModuleBlueprint);
  return *this;
}

ShipBlueprintPtr BlueprintsStore::makeCommandCenterBlueprint()
{
  return ShipBlueprint()
      .setShipType("CommandCenter")
      .setWeight(4000000)
      .wrapToSharedPtr();
}

ShipBlueprintPtr BlueprintsStore::makeCorvetBlueprint()
{
  return ShipBlueprint()
      .setShipType("Corvet")
      .setWeight(250000)
      .addModule(modules::EngineBlueprint().setMaxThrust(5000000).wrapToSharedPtr())
      .wrapToSharedPtr();
}

ShipBlueprintPtr BlueprintsStore::makeMinerBlueprint()
{
  return ShipBlueprint()
      .setShipType("Miner")
      .setWeight(100000)
      .addModule(modules::EngineBlueprint().setMaxThrust(5000000).wrapToSharedPtr())
      .wrapToSharedPtr();
}

ShipBlueprintPtr BlueprintsStore::makeZondBlueprint()
{
  return ShipBlueprint()
      .setShipType("Zond")
      .setWeight(5000)
      .addModule(modules::EngineBlueprint().setMaxThrust(100000).wrapToSharedPtr())
      .wrapToSharedPtr();
}


} // namespace ships

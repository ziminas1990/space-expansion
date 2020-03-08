#include "ResourceCollecting.h"

#include <float.h>

#include <Ships/Ship.h>
#include <Modules/Shipyard/Shipyard.h>
#include <Modules/ResourceContainer/ResourceContainer.h>
#include <Utils/YamlReader.h>

namespace arbitrator {

ResourceCollecting::ResourceCollecting(world::PlayerStoragePtr pPlayersStorage)
  : BaseArbitrator(pPlayersStorage)
{}

bool ResourceCollecting::loadConfiguation(YAML::Node const& data)
{
  utils::YamlReader reader(data);
  return BaseArbitrator::loadConfiguation(data)
      && reader.read("resources", m_target);
}

uint32_t ResourceCollecting::score(world::PlayerPtr pPlayer)
{
  // Calculating total amount of resources, that are stored in containers, that
  // are connected to shipyards.

  world::ResourcesArray totalResources;
  totalResources.fill(0);

  for (modules::BaseModulePtr const& pModule
       : pPlayer->getCommutator()->getAllModules()) {
    ships::ShipPtr pShip = std::dynamic_pointer_cast<ships::Ship>(pModule);
    if (!pShip)
      continue;

    for (modules::BaseModulePtr const& pShipModule :
         pShip->getCommutator()->getAllModules()) {
      if (pShipModule->getModuleType() != modules::Shipyard::TypeName())
        continue;

      modules::ShipyardPtr const& pShipyard =
          std::static_pointer_cast<modules::Shipyard>(pShipModule);

      std::string const& sContainerName = pShipyard->getContainerName();
      modules::ResourceContainerPtr const& pContainer =
          std::dynamic_pointer_cast<modules::ResourceContainer>(
            pShip->getCommutator()->findModuleByName(sContainerName));

      if (pContainer) {
        totalResources += pContainer->getResources();
      }
    }
  }

  double   summ  = 0;
  uint32_t parts = 0;
  for (world::Resource::Type eType : world::Resource::AllTypes) {
    if (m_target[eType] <= 0)
      continue;
    double progress = totalResources[eType] / m_target[eType];
    summ += std::min(progress, 1.0);
    ++parts;
  }

  double progress = summ / parts;
  return static_cast<uint32_t>(getTargetScore() * progress * (1 + DBL_EPSILON));
}

} // namespace arbitrator


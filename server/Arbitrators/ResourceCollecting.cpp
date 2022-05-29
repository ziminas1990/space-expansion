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

uint32_t ResourceCollecting::score(world::PlayerPtr)
{
  // Calculating total amount of resources, that are stored in containers in
  // all ships, that have a shipyard

  world::ResourcesArray totalResources;

  // for (modules::BaseModulePtr const& pModule
  //      : pPlayer->getCommutator()->getAllModules()) {
  //   ships::ShipPtr pShip = std::dynamic_pointer_cast<ships::Ship>(pModule);
  //   if (!pShip)
  //     continue;

  //   world::ResourcesArray shipResources;
  //   bool hasShipyard = false;
  //   for (modules::BaseModulePtr const& pShipModule :
  //        pShip->getCommutator()->getAllModules()) {

  //     std::string const& moduleType = pShipModule->getModuleType();

  //     if (moduleType == modules::ResourceContainer::TypeName()) {
  //       modules::ResourceContainerPtr pContainer =
  //           std::static_pointer_cast<modules::ResourceContainer>(pShipModule);
  //       shipResources += pContainer->getResources();
  //     } else if (moduleType == modules::Shipyard::TypeName()) {
  //       hasShipyard = true;
  //     }
  //   }
  //   if (hasShipyard) {
  //     totalResources += shipResources;
  //   }
  // }

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


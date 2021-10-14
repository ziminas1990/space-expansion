#include "Resources.h"

#include <memory>

#include <Autotests/ClientSDK/Modules/ClientResourceContainer.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

namespace autotests { namespace client {

bool ResourcesManagment::shift(Ship& ship,
                               std::string const& sSourceName,
                               std::string const& sDestinationName,
                               world::ResourcesArray const& resources)
{
  return transfer(ship, sSourceName, ship, sDestinationName, resources);
}

bool ResourcesManagment::transfer(
    Ship& sourceShip, std::string const& sSourceContainer,
    Ship& destShip, std::string const& sDestContainer,
    world::ResourcesArray const& resources)
{
  static const uint32_t nAccessKey = 1334;
  ResourceContainer     source;
  ResourceContainer     destination;
  uint32_t              nPortId = 0;
  if (!FindResourceContainer(sourceShip, source, sSourceContainer))
    return false;

  if (!FindResourceContainer(destShip, destination, sDestContainer))
    return false;

  if (destination.openPort(nAccessKey, nPortId) != ResourceContainer::eStatusOk)
    return false;

  for (world::Resource::Type type : world::Resource::MaterialResources) {
    if (resources[type] > 0.0) {
      if (source.transferRequest(nPortId, nAccessKey, type, resources[type]) !=
          ResourceContainer::eStatusOk)
        return false;
      if (source.waitTransfer(resources[type]) != ResourceContainer::eStatusOk)
        return false;
    }
  }
  return destination.closePort() == ResourceContainer::eStatusOk;
}

}}  // namespace autotests::client


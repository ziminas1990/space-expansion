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
  static const uint32_t nAccessKey = 1334;
  ResourceContainer     source;
  ResourceContainer     destination;
  uint32_t              nPortId;
  if (!FindResourceContainer(ship, source, sSourceName))
    return false;

  if (!FindResourceContainer(ship, destination, sDestinationName))
    return false;

  if (destination.openPort(nAccessKey, nPortId) != ResourceContainer::eStatusOk)
    return false;

  for (world::Resource::Type type : world::Resource::MaterialResources) {
    if (resources[type] > 0.0) {
      if (source.transferRequest(nPortId, nAccessKey, type, resources[type]) !=
          ResourceContainer::eStatusOk)
        return false;
      if (source.waitTransfer(type, resources[type]) != ResourceContainer::eStatusOk)
        return false;
    }
  }
  return true;
}

}}  // namespace autotests::client


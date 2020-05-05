#pragma once

#include <stdint.h>

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include "AbstractProcedure.h"
#include <World/Resources.h>

namespace autotests { namespace client {

class ResourcesManagment
{
public:
  static bool shift(Ship& ship,
                    std::string const& sSourceName,
                    std::string const& sDestinationName,
                    world::ResourcesArray const& resources);
    // Moves the specified 'resources' from container with the specified 'sSourceName' to
    // container with the specified 'sDestinationName'. Both containers should be a part
    // of the specified 'pShip'

  static bool transfer(Ship& sourceShip, std::string const& sSourceContainer,
                       Ship& destShip, std::string const& sDestContainer,
                       world::ResourcesArray const& resources);
};

}} // namespace autotests::client

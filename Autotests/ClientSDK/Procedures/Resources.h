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
                    const world::ResourcesArray &resources);
    // Moves the specified 'resources' from container with the specified 'sSourceName' to
    // container with the specified 'sDestinationName'. Both containers should be a part
    // of the specified 'pShip'
};

}} // namespace autotests::client

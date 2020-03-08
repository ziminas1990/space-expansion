#pragma once

#include "BaseArbitrator.h"
#include <World/PlayersStorage.h>

namespace arbitrator {

class Factory
{
public:
  static BaseArbitratorPtr make(YAML::Node const& configuration,
                                world::PlayerStoragePtr pPlayersStorage);
};

} // namespace arbitrator

#pragma once

#include <memory>
#include <string>
#include <map>

#include "Player.h"
#include <Utils/Mutex.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Commutator/Commutator.h>
#include "Blueprints/Modules/BlueprintsLibrary.h"
#include "Blueprints/Ships/ShipBlueprintsLibrary.h"

#include <Utils/YamlForwardDeclarations.h>


namespace world {

class PlayersStorage
{
public:
  bool loadState(YAML::Node const& data,
                 modules::BlueprintsLibrary   const& avaliableModulesBlueprints,
                 ships::ShipBlueprintsLibrary const& shipsBlueprints);

  PlayerPtr getPlayer(std::string const& sLogin) const;

private:
  std::map<std::string, PlayerPtr> m_players;

  mutable utils::Mutex m_Mutex;
};

using PlayerStoragePtr     = std::shared_ptr<PlayersStorage>;
using PlayerStorageWeakPtr = std::weak_ptr<PlayersStorage>;

} // namespace World

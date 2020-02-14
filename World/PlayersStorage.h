#pragma once

#include <memory>
#include <string>
#include <map>

#include "Player.h"
#include <Utils/Mutex.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Commutator/Commutator.h>
#include "Blueprints/BlueprintsLibrary.h"

#include <Utils/YamlForwardDeclarations.h>


namespace world {

class PlayersStorage
{
public:
  bool loadState(YAML::Node const& data, blueprints::BlueprintsLibrary const& blueprints);

  PlayerPtr getPlayer(std::string const& sLogin) const;

private:
  std::map<std::string, PlayerPtr> m_players;

  mutable utils::Mutex m_Mutex;
};

using PlayerStoragePtr     = std::shared_ptr<PlayersStorage>;
using PlayerStorageWeakPtr = std::weak_ptr<PlayersStorage>;

} // namespace World

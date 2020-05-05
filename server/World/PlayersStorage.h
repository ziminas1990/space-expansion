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

  std::vector<PlayerPtr> const& getAllPlayers() const { return m_players; }
  PlayerPtr getPlayer(std::string const& sLogin) const;

private:
  std::vector<PlayerPtr> m_players;
  std::map<std::string, size_t> m_loginToPlayer;

  mutable utils::Mutex m_Mutex;
};

using PlayerStoragePtr     = std::shared_ptr<PlayersStorage>;
using PlayerStorageWeakPtr = std::weak_ptr<PlayersStorage>;

} // namespace World

#pragma once

#include <memory>
#include <string>
#include <map>

#include "Player.h"
#include <Utils/Mutex.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Commutator/Commutator.h>
#include <Blueprints/BlueprintsStorage.h>
#include <Utils/YamlForwardDeclarations.h>

namespace world {

class PlayersStorage
{
public:
  void attachToBlueprintsStorage(blueprints::BlueprintsStoragePtr pStorage);
  bool loadState(YAML::Node const& data);

  PlayerPtr getPlayer(std::string const& sLogin) const;

private:
  blueprints::BlueprintsStoragePtr m_pBlueprintsStorage;

  std::map<std::string, PlayerPtr> m_players;

  mutable utils::Mutex m_Mutex;
};

using PlayerStoragePtr     = std::shared_ptr<PlayersStorage>;
using PlayerStorageWeakPtr = std::weak_ptr<PlayersStorage>;

} // namespace World

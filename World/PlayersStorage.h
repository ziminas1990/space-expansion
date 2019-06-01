#pragma once

#include <memory>
#include <string>
#include <map>

#include <Utils/Mutex.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Commutator/Commutator.h>
#include <Ships/ShipsManager.h>
#include <Blueprints/BlueprintsStorage.h>

namespace world {

class PlayersStorage
{
  struct PlayerInfo
  {
    network::ProtobufChannelPtr   m_pChannel;
    modules::CommutatorPtr        m_pEntryPoint;
    std::vector<ships::ShipPtr>   m_ships;
  };

public:
  ~PlayersStorage();

  void attachToBlueprintsStorage(blueprints::BlueprintsStoragePtr pStorage);

  modules::CommutatorPtr getPlayer(std::string const& sLogin) const;
  modules::CommutatorPtr spawnPlayer(
      std::string const& sLogin, network::ProtobufChannelPtr pChannel);

private:
  PlayerInfo createNewPlayer(network::ProtobufChannelPtr pChannel);
  void       kickPlayer(PlayerInfo& player);

private:
  blueprints::BlueprintsStoragePtr m_pBlueprintsStorage;

  // Login -> CommandCenter
  std::map<std::string, PlayerInfo> m_players;

  mutable utils::Mutex m_Mutex;
};

using PlayerStoragePtr     = std::shared_ptr<PlayersStorage>;
using PlayerStorageWeakPtr = std::weak_ptr<PlayersStorage>;

} // namespace World

#pragma once

#include <memory>
#include <string>
#include <map>

#include <Utils/Mutex.h>
#include <Network/ProtobufChannel.h>
#include <Modules/Commutator/Commutator.h>
#include <Ships/ShipsManager.h>
#include <Ships/CommandCenter.h>
#include <Ships/Corvet.h>
#include <Ships/Zond.h>
#include <Ships/Miner.h>

namespace world {

class PlayersStorage
{
  struct PlayerInfo
  {
    network::ProtobufChannelPtr   m_pChannel;
    modules::CommutatorPtr        m_pEntryPoint;
    ships::CommandCenterPtr       m_pCommandCenter;
    // Probably, there is no need to separate different types of ships to
    // different containers
    std::vector<ships::MinerPtr>  m_miners;
    std::vector<ships::CorvetPtr> m_corvets;
    std::vector<ships::ZondPtr>   m_zonds;
  };

public:
  ~PlayersStorage();

  void attachToShipManager(ships::ShipsManagerWeakPtr pManager);

  modules::CommutatorPtr getPlayer(std::string const& sLogin) const;
  modules::CommutatorPtr spawnPlayer(
      std::string const& sLogin, network::ProtobufChannelPtr pChannel);


private:
  PlayerInfo createNewPlayer(network::ProtobufChannelPtr pChannel);
  void       kickPlayer(PlayerInfo& player);

private:
  // Login -> CommandCenter
  std::map<std::string, PlayerInfo> m_players;
  ships::ShipsManagerWeakPtr m_pShipsManager;

  // TODO: replace with spinlock?
  mutable utils::Mutex m_Mutex;
};

using PlayerStoragePtr     = std::shared_ptr<PlayersStorage>;
using PlayerStorageWeakPtr = std::weak_ptr<PlayersStorage>;

} // namespace World

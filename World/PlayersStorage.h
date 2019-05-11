#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <map>

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
    modules::CommutatorPtr        m_pEntryPoint;
    ships::CommandCenterPtr       m_pCommandCenter;
    // Probably, there is no need to separate different types of ships to
    // different containers
    std::vector<ships::MinerPtr>  m_miners;
    std::vector<ships::CorvetPtr> m_corvets;
    std::vector<ships::ZondPtr>   m_zonds;
  };

public:
  void attachToShipManager(ships::ShipsManagerWeakPtr pManager);

  modules::CommutatorPtr getOrSpawnPlayer(std::string const& sLogin);

private:
  PlayerInfo spawnPlayer();

private:
  // Login -> CommandCenter
  std::map<std::string, PlayerInfo> m_players;
  ships::ShipsManagerWeakPtr m_pShipsManager;

  // TODO: replace with spinlock?
  std::mutex m_Mutex;
};

using PlayerStoragePtr     = std::shared_ptr<PlayersStorage>;
using PlayerStorageWeakPtr = std::weak_ptr<PlayersStorage>;

} // namespace World

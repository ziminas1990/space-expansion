#include "PlayersStorage.h"
#include <Ships/ShipBlueprint.h>

namespace world {

PlayersStorage::~PlayersStorage()
{
  for (auto& player : m_players) {
    kickPlayer(player.second);
  }
}

modules::CommutatorPtr PlayersStorage::getPlayer(std::string const& sLogin) const
{
  std::lock_guard<utils::Mutex> guard(m_Mutex);
  auto I = m_players.find(sLogin);
  return (I != m_players.end()) ? I->second.m_pEntryPoint : modules::CommutatorPtr();
}

modules::CommutatorPtr PlayersStorage::spawnPlayer(
    std::string const& sLogin, network::ProtobufChannelPtr pChannel)
{
  std::lock_guard<utils::Mutex> guard(m_Mutex);

  PlayerInfo info = createNewPlayer(pChannel);
  modules::CommutatorPtr pEntryPoint = info.m_pEntryPoint;
  m_players.insert(std::make_pair(sLogin, std::move(info)));
  return pEntryPoint;
}

PlayersStorage::PlayerInfo PlayersStorage::createNewPlayer(
    network::ProtobufChannelPtr pChannel)
{
  PlayerInfo info;

  // TODO SES-20: thread safe commutator should be used here!
  info.m_pEntryPoint = std::make_shared<modules::Commutator>();
  info.m_pChannel    = pChannel;
  info.m_pChannel->attachToTerminal(info.m_pEntryPoint);
  info.m_pEntryPoint->attachToChannel(info.m_pChannel);

  // Creating all ships:
  info.m_ships.reserve(0xFF);
  info.m_ships.push_back(ships::BlueprintsStore::makeCommandCenterBlueprint()->build());
  for(size_t i = 0; i < 2; ++i) {
    info.m_ships.push_back(ships::BlueprintsStore::makeCorvetBlueprint()->build());
    info.m_ships.push_back(ships::BlueprintsStore::makeMinerBlueprint()->build());
    info.m_ships.push_back(ships::BlueprintsStore::makeZondBlueprint()->build());
  }

  // Adding ships to Commutator:
  for (ships::ShipPtr pShip : info.m_ships) {
    info.m_pEntryPoint->attachModule(pShip);
    pShip->attachToChannel(info.m_pEntryPoint);
  }
  return info;
}

void PlayersStorage::kickPlayer(PlayersStorage::PlayerInfo& player)
{
  player.m_pEntryPoint->detachFromModules();
  player.m_pEntryPoint->detachFromChannel();
  player.m_pChannel->detachFromTerminal();
  player.m_pChannel->detachFromChannel();
}

} // namespace world

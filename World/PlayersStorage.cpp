#include "PlayersStorage.h"
#include <Ships/CommandCenter.h>
#include <Ships/ShipsManager.h>

namespace world {

PlayersStorage::~PlayersStorage()
{
  for (auto& player : m_players) {
    kickPlayer(player.second);
  }
}

void PlayersStorage::attachToManagersHive(ManagersHivePtr pManagersHive)
{
  m_pManagersHive = pManagersHive;
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
  ships::ShipsManagerPtr pShipsManager = m_pManagersHive->m_pShipsManager;

  PlayerInfo info = createNewPlayer(pChannel);
  pShipsManager->addNewOne(info.m_pCommandCenter);
  for (ships::ShipPtr pSomeShip : info.m_miners)
    pShipsManager->addNewOne(pSomeShip);
  for (ships::ShipPtr pSomeShip : info.m_zonds)
    pShipsManager->addNewOne(pSomeShip);
  for (ships::ShipPtr pSomeShip : info.m_corvets)
    pShipsManager->addNewOne(pSomeShip);

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
  info.m_pCommandCenter = std::make_shared<ships::CommandCenter>();
  info.m_miners.reserve(0x80);
  info.m_corvets.reserve(0x80);
  info.m_zonds.reserve(0x80);

  for(size_t i = 0; i < 2; ++i) {
    info.m_miners.push_back(std::make_shared<ships::Miner>());
    info.m_corvets.push_back(std::make_shared<ships::Corvet>());
    info.m_zonds.push_back(std::make_shared<ships::Zond>());
  }

  // Adding ships to Commutator:
  info.m_pEntryPoint->attachModule(info.m_pCommandCenter);
  for (ships::ShipPtr pSomeShip : info.m_miners) {
    info.m_pEntryPoint->attachModule(pSomeShip);
    pSomeShip->attachToChannel(info.m_pEntryPoint);
  }
  for (ships::ShipPtr pSomeShip : info.m_zonds) {
    info.m_pEntryPoint->attachModule(pSomeShip);
    pSomeShip->attachToChannel(info.m_pEntryPoint);
  }
  for (ships::ShipPtr pSomeShip : info.m_corvets) {
    info.m_pEntryPoint->attachModule(pSomeShip);
    pSomeShip->attachToChannel(info.m_pEntryPoint);
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

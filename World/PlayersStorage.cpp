#include "PlayersStorage.h"
#include <Ships/CommandCenter.h>
#include <Ships/ShipsManager.h>

namespace world {

void PlayersStorage::attachToShipManager(ships::ShipsManagerWeakPtr pManager)
{
  m_pShipsManager = pManager;
}

modules::CommutatorPtr PlayersStorage::getOrSpawnPlayer(std::string const& sLogin)
{
  std::lock_guard<std::mutex> guard(m_Mutex);
  auto I = m_players.find(sLogin);
  if (I != m_players.end())
    return I->second.m_pEntryPoint;

  ships::ShipsManagerPtr pShipsManager = m_pShipsManager.lock();
  if (!pShipsManager)
    return modules::CommutatorPtr();

  PlayerInfo info = spawnPlayer();
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

PlayersStorage::PlayerInfo PlayersStorage::spawnPlayer()
{
  PlayerInfo info;

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

  // TODO SES-20: thread safe commutator should be used here!
  info.m_pEntryPoint = std::make_shared<modules::Commutator>();

  // Adding ships to Commutator:
  for (ships::ShipPtr pSomeShip : info.m_miners)
    info.m_pEntryPoint->attachModule(pSomeShip);
  for (ships::ShipPtr pSomeShip : info.m_zonds)
    info.m_pEntryPoint->attachModule(pSomeShip);
  for (ships::ShipPtr pSomeShip : info.m_corvets)
    info.m_pEntryPoint->attachModule(pSomeShip);

  return info;
}

} // namespace world

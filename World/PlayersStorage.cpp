#include "PlayersStorage.h"
#include <Ships/CommandCenter.h>
#include <Ships/ShipsManager.h>

namespace world {

void PlayerStorage::attachToCommandCenterManager(ships::ShipsManagerWeakPtr pManager)
{
  m_pShipsManager = pManager;
}

ships::CommandCenterPtr
PlayerStorage::getOrCreateCommandCenter(std::string const& sLogin)
{
  std::lock_guard<std::mutex> guard(m_Mutex);
  auto I = m_Players.find(sLogin);
  if (I != m_Players.end())
    return I->second;

  ships::ShipsManagerPtr pManager = m_pShipsManager.lock();
  if (!pManager)
    return ships::CommandCenterPtr();

  ships::CommandCenterPtr pNewCommandCenter = std::make_shared<ships::CommandCenter>();
  pManager->addNewOne(pNewCommandCenter);

  m_Players.insert(std::make_pair(sLogin, pNewCommandCenter));
  return pNewCommandCenter;
}

} // namespace world

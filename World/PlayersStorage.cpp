#include "PlayersStorage.h"
#include <Modules/CommandCenter/CommandCenter.h>
#include <Modules/CommandCenter/CommanCenterManager.h>

namespace world {

void PlayerStorage::attachToCommandCenterManager(
    modules::CommandCenterManagerWeakPtr pManager)
{
  m_pCommandCenterManager = pManager;
}

modules::CommandCenterPtr
PlayerStorage::getOrCreateCommandCenter(std::string&& sLogin)
{
  std::lock_guard<std::mutex> guard(m_Mutex);
  auto I = m_Players.find(sLogin);
  if (I != m_Players.end())
    return I->second;

  modules::CommandCenterManagerPtr pManager = m_pCommandCenterManager.lock();
  if (!pManager)
    return modules::CommandCenterPtr();

  modules::CommandCenterPtr pNewCommandCenter =
      std::make_shared<modules::CommandCenter>();
  pManager->addNewOne(pNewCommandCenter);

  m_Players.insert(std::make_pair(std::move(sLogin), pNewCommandCenter));
  return pNewCommandCenter;
}

} // namespace world

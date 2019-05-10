#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <map>

namespace ships {
class CommandCenter;
using CommandCenterPtr     = std::shared_ptr<CommandCenter>;

class ShipsManager;
using ShipsManagerWeakPtr = std::weak_ptr<ShipsManager>;
}

namespace world {

class PlayerStorage
{
public:
  void attachToCommandCenterManager(ships::ShipsManagerWeakPtr pManager);

  // Returns command canter for player with login "sLogin".
  ships::CommandCenterPtr getOrCreateCommandCenter(std::string const& sLogin);

private:
  // Login -> CommandCenter
  std::map<std::string, ships::CommandCenterPtr> m_Players;
  ships::ShipsManagerWeakPtr m_pShipsManager;

  std::mutex m_Mutex;
};

using PlayerStoragePtr     = std::shared_ptr<PlayerStorage>;
using PlayerStorageWeakPtr = std::weak_ptr<PlayerStorage>;

} // namespace World

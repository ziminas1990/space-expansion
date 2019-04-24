#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <map>

namespace modules {
class CommandCenter;
using CommandCenterPtr     = std::shared_ptr<CommandCenter>;

class CommandCenterManager;
using CommandCenterManagerWeakPtr = std::weak_ptr<CommandCenterManager>;
}

namespace world {

class PlayerStorage
{
public:
  void attachToCommandCenterManager(modules::CommandCenterManagerWeakPtr pManager);

  // Returns command canter for player with login "sLogin".
  modules::CommandCenterPtr getOrCreateCommandCenter(std::string&& sLogin);

private:
  // Login -> CommandCenter
  std::map<std::string, modules::CommandCenterPtr> m_Players;
  modules::CommandCenterManagerWeakPtr m_pCommandCenterManager;

  std::mutex m_Mutex;
};

using PlayerStoragePtr     = std::shared_ptr<PlayerStorage>;
using PlayerStorageWeakPtr = std::weak_ptr<PlayerStorage>;

} // namespace World

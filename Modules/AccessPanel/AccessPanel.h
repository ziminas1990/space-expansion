#pragma once

#include <memory>
#include <Network/ConnectionManager.h>
#include <Network/BufferedTerminal.h>
#include <World/PlayersStorage.h>

namespace world {

class PlayerStorage;
using PlayerStorageWeakPtr = std::weak_ptr<PlayerStorage>;

} // namespace world


namespace modules {

class AccessPanel : public network::BufferedTerminal
{
public:
  void attachToConnectionManager(network::ConnectionManagerPtr pManager)
  { m_pConnectionManager = pManager; }

  void attachToPlayerStorage(world::PlayerStorageWeakPtr pPlayersStorage)
  { m_pPlayersStorage = pPlayersStorage; }

protected:
  // overrides from BufferedTerminal interface
  void handleMessage(network::MessagePtr pMessage, size_t nLength) override;

private:
  bool checkLogin(std::string const& sLogin, std::string const& nPassword);

  void sendLoginSuccess(network::UdpEndPoint const& localAddress);
  void sendLoginFailed(std::string const& reason);

private:
  network::ConnectionManagerPtr m_pConnectionManager;
  world::PlayerStorageWeakPtr   m_pPlayersStorage;
};

using AccessPanelPtr = std::shared_ptr<AccessPanel>;


class AccessPanelFacotry : public network::IBufferedTerminalFactory
{
public:

  void setCreationData(network::ConnectionManagerPtr pManager,
                       world::PlayerStorageWeakPtr   pPlayersStorage);

  // overrides from IBufferedTerminalFactory interface
  network::BufferedTerminalPtr make();

private:
  network::ConnectionManagerPtr m_pManager;
  world::PlayerStorageWeakPtr   m_pPlayersStorage;
};

using AccessPanelFacotryPtr = std::shared_ptr<AccessPanelFacotry>;

} // namespace modules

#pragma once

#include <memory>
#include <Network/UdpDispatcher.h>
#include <Network/BufferedProtobufTerminal.h>
#include <World/PlayersStorage.h>

namespace world {

class PlayersStorage;
using PlayerStorageWeakPtr = std::weak_ptr<PlayersStorage>;

} // namespace world


namespace modules {

class AccessPanel : public network::BufferedProtobufTerminal
{
public:
  void attachToConnectionManager(network::UdpDispatcherPtr pManager)
  { m_pConnectionManager = pManager; }

  void attachToPlayerStorage(world::PlayerStorageWeakPtr pPlayersStorage)
  { m_pPlayersStorage = pPlayersStorage; }

  // from BufferedTerminal->IBinaryTerminal interface:
  bool openSession(uint32_t /*nSessionId*/) override { return true; }
  void onSessionClosed(uint32_t /*nSessionId*/) override {}

protected:
  // overrides from BufferedTerminal interface
  void handleMessage(uint32_t nSessionId, spex::Message const& message) override;

private:
  bool checkLogin(std::string const& sLogin, std::string const& nPassword);

  bool sendLoginSuccess(uint32_t nSessionId, network::UdpEndPoint const& localAddress);
  bool sendLoginFailed(uint32_t nSessionId, std::string const& reason);

private:
  network::UdpDispatcherPtr m_pConnectionManager;
  world::PlayerStorageWeakPtr   m_pPlayersStorage;
};

using AccessPanelPtr = std::shared_ptr<AccessPanel>;

} // namespace modules

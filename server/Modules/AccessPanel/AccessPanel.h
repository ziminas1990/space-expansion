#pragma once

#include <memory>
#include <Network/UdpDispatcher.h>
#include <Network/BufferedProtobufTerminal.h>
#include <World/PlayersStorage.h>
#include <Conveyor/IAbstractLogic.h>

namespace world {

class PlayersStorage;
using PlayerStorageWeakPtr = std::weak_ptr<PlayersStorage>;

} // namespace world


namespace modules {

class AccessPanel :
    public network::BufferedPlayerTerminal,
    public conveyor::IAbstractLogic
{
public:

  void attachToLoginSocket(network::UdpSocketPtr pLoginSocket)
  { m_pLoginSocket = pLoginSocket; }

  void attachToConnectionManager(network::UdpDispatcherPtr pManager)
  { m_pConnectionManager = pManager; }

  void attachToPlayerStorage(world::PlayerStorageWeakPtr pPlayersStorage)
  { m_pPlayersStorage = pPlayersStorage; }

  // from BufferedTerminal->IBinaryTerminal interface:
  bool openSession(uint32_t /*nSessionId*/) override { return true; }
  void onSessionClosed(uint32_t /*nSessionId*/) override {}

  // from IAbstractLogic interface
  uint16_t getStagesCount() override { return 1; }
  bool     prephare(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override;
  void     proceed(uint16_t, uint32_t, uint64_t) override {}
  size_t   getCooldownTimeUs() const override {
#ifndef AUTOTESTS_MODE
    return 10000;
#else
    // In autotests mode we can't afford to let logics to sleep for unpredictable period
    // of time
    return 0;
#endif
  }

protected:
  // overrides from BufferedTerminal interface
  void handleMessage(uint32_t nSessionId, spex::Message const& message) override;

private:
  bool checkLogin(std::string const& sLogin, std::string const& nPassword) const;

  bool sendLoginSuccess(uint32_t nSessionId,
                        uint32_t nRootSessionId,
                        network::UdpEndPoint const& localAddress);
  bool sendLoginFailed(uint32_t nSessionId, std::string const& reason);

private:
  network::UdpSocketPtr         m_pLoginSocket;
  network::UdpDispatcherPtr     m_pConnectionManager;
  world::PlayerStorageWeakPtr   m_pPlayersStorage;
};

} // namespace modules

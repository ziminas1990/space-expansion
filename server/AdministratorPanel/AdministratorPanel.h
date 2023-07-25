#pragma once

#include <map>
#include <memory>

#include <Network/BufferedProtobufTerminal.h>
#include <Conveyor/IAbstractLogic.h>
#include <Network/Fwd.h>
#include <ConfigDI/Containers.h>
#include <Utils/RandomSequence.h>

#include <AdministratorPanel/ClockControl.h>
#include <AdministratorPanel/Screen.h>
#include <AdministratorPanel/Spawn.h>
#include <AdministratorPanel/BasicManipulator.h>

class SystemManager;

class AdministratorPanel :
    public network::BufferedPrivilegedTerminal,
    public conveyor::IAbstractLogic
{
public:
  AdministratorPanel(config::IAdministratorCfg const& cfg,
                     unsigned int nTokenPattern);

  void attachToSystemManager(SystemManager* pSystemManager);

  void attachToAdminSocket(network::UdpSocketPtr pSocket) {
    m_pAdminSocket = pSocket;
  }

  // from BufferedPrivilegedTerminal->IBinaryTerminal interface:
  bool canOpenSession() const override { return true; }
  void openSession(uint32_t /*nSessionId*/) override {}
  void onSessionClosed(uint32_t /*nSessionId*/) override {}

  // from IAbstractLogic interface
  uint16_t getStagesCount() override { return 1; }
  bool     prephare(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override;
  void     proceed(uint16_t, uint32_t, uint64_t) override {}
  size_t   getCooldownTimeUs() const override { return 0; }

protected:
  // from network::BufferedPrivilegedTerminal
  void handleMessage(uint32_t nSessionId, admin::Message const& message) override;

private:
  // Functions to handle the "Access" interface
  void onLoginRequest(uint32_t nSessionId, admin::Access::Login const& message);
  void sendLoginSuccess(uint32_t nSessionId, uint64_t nToken);
  void sendLoginFailed(uint32_t nSessionId);

private:
  config::AdministratorCfg        m_cfg;
  SystemManager*                  m_pSystemManager;
  network::UdpSocketPtr           m_pAdminSocket;
  administrator::ClockControl     m_clockControl;
  administrator::Screen           m_screen;
  administrator::SpawnLogic       m_spawner;
  administrator::BasicManipulator m_manipulator;

  utils::RandomSequence m_tokenGenerator;

  std::map<uint32_t, uint64_t> m_tokens;
    // Key is a sessionID, value is a token
};

using AdministratorPanelPtr = std::shared_ptr<AdministratorPanel>;

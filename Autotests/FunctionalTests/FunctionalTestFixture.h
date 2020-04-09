#pragma once

#include <gtest/gtest.h>
#include <ConfigDI/Interfaces.h>
#include <ConfigDI/Containers.h>
#include <SystemManager.h>

#include <Autotests/ClientSDK/Socket.h>
#include <Autotests/ClientSDK/SyncPipe.h>
#include <Autotests/ClientSDK/Modules/ClientAccessPanel.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>
#include <Autotests/ClientSDK/Modules/ClientAdministratorPanel.h>

#include <Utils/YamlForwardDeclarations.h>

namespace autotests
{

// Base class for all functional test fixtures
// It does:
// 1. creates application with specified configuration (initializing SystemManager)
// 2. creates basic components stack on client side
class FunctionalTestFixture : public ::testing::Test
{
  friend class Scenarios;
public:
  FunctionalTestFixture();

  void SetUp() override;
  void TearDown() override;

protected:
  virtual config::ApplicationCfg prephareConfiguration();
  virtual bool initialWorldState(YAML::Node& /*state*/) { return false; }

  void pauseTime();
  void resumeTime();
  void skipTime(uint32_t nTimeMs, uint32_t nTickUs = 10000);

  void proceed();

protected:
  config::ApplicationCfg m_cfg;
  SystemManager          m_application;
  std::thread*           m_pMainThread = nullptr;

  bool m_lWorldFreezed;

  boost::asio::ip::udp::endpoint m_clientAddress;
    // UDP address, that will be used to connect as client
  boost::asio::ip::udp::endpoint m_adminAddress;
    // UDP address, that will be used to connect as admin
  boost::asio::ip::udp::endpoint m_serverLoginAddress;
    // Address, which is used by server to accept login requests
  boost::asio::ip::udp::endpoint m_serverAddress;
    // Address, that was returned by server after successfull login
  boost::asio::ip::udp::endpoint m_serverPrivilegedAddress;
    // Adress, that is used by server to receive commands from
    // administrator

  // Components for client side:
  //                                        Tunnels and Mocked Modules...
  //                                                     |
  //     +-------------------+               +----------------------+
  //     | ClientAccessPoint |               | RootClientCommutator |
  //     +-------------------+               +----------------------+
  //               |                                     |
  //               +----------------+   +----------------+
  //                                |   |
  //                      +------------------------+
  //                      |   client::PlayerPipe   |
  //                      +------------------------+
  //                                  |
  //                      +------------------------+
  //                      |  client::PlayerSocket  |
  //                      +------------------------+
  //                                  |
  //                                  +-------------------------------> Server
  //

  boost::asio::io_service      m_IoService;
  client::PlayerSocketPtr      m_pSocket;
  client::PlayerPipePtr        m_pRootPipe;
  client::ClientAccessPanelPtr m_pAccessPanel;
  client::ClientCommutatorPtr  m_pRootCommutator;

  // Components to communicate with access panel
  client::PrivilegedSocketPtr   m_pPrivilegedSocket;
  client::PrivilegedPipePtr     m_pPrivilegedPipe;
  client::AdministratorPanelPtr m_pAdminPanel;
  uint64_t                      m_nAdminToken;
};

} // namespace autotests

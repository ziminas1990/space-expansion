#pragma once

#include <gtest/gtest.h>
#include <ConfigDI/Interfaces.h>
#include <ConfigDI/Containers.h>
#include <SystemManager.h>

#include <Autotests/ClientSDK/Socket.h>
#include <Autotests/ClientSDK/SyncPipe.h>
#include <Autotests/ClientSDK/Modules/ClientAccessPanel.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>

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

  void freezeWorld()  { m_lWorldFreezed = true; }
  void animateWorld() { m_lWorldFreezed = false; }

  void proceedFreezedWorld();
  void proceedEnviroment(uint32_t nMilliseconds, uint32_t nTickUs = 500);

protected:
  config::ApplicationCfg m_cfg;
  SystemManager          m_application;

  bool m_lWorldFreezed;

  boost::asio::ip::udp::endpoint m_clientAddress;
  boost::asio::ip::udp::endpoint m_serverLoginAddress;
  boost::asio::ip::udp::endpoint m_serverAddress;

  // Components for client side:
  //                                        Tunnels and Mocked Modules...
  //                                                     |
  //     +-------------------+               +----------------------+
  //     | ClientAccessPoint |               | RootClientCommutator |
  //     +-------------------+               +----------------------+
  //               |                                     |
  //               +----------------+   +----------------+
  //                                |   |
  //                        +---------------------+
  //                        |  client::SyncPipe   |
  //                        +---------------------+
  //                                  |
  //                        +---------------------+
  //                        |   client::Socket    |
  //                        +---------------------+
  //                                  |
  //                                  +-------------------------------> Server
  //

  boost::asio::io_service         m_IoService;
  client::SocketPtr               m_pSocket;
  client::SyncPipePtr             m_pRootPipe;
  client::ClientAccessPanelPtr    m_pAccessPanel;
  client::ClientCommutatorPtr     m_pRootCommutator;
};

} // namespace autotests

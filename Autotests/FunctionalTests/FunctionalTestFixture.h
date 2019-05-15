#pragma once

#include <gtest/gtest.h>
#include <ConfigDI/Interfaces.h>
#include <ConfigDI/Containers.h>
#include <SystemManager.h>
#include <Network/UdpDispatcher.h>
#include <Network/ProtobufChannel.h>
#include <Autotests/TestUtils/ProtobufSyncPipe.h>
#include <Autotests/TestUtils/ClientUdpSocket.h>
#include <Autotests/Mocks/Modules/MockedAccessPoint.h>
#include <Autotests/Mocks/Modules/MockedCommutator.h>

namespace autotests
{

// Base class for all functional test fixtures
// It does:
// 1. creates application with specified configuration (initializing SystemManager)
// 2. creates basic components stack on client side
class FunctionalTestFixture : public ::testing::Test
{
public:
  void SetUp() override;
  void TearDown() override;

protected:
  virtual config::ApplicationCfg prephareConfiguration();

  void proceedEnviroment(uint32_t nMilliseconds);

protected:
  config::ApplicationCfg m_cfg;
  SystemManager          m_application;

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
  //                        | SyncProtobufChannel |
  //                        +---------------------+
  //                                  |
  //                        +---------------------+
  //                        |   ProtobufChannel   |
  //                        +---------------------+
  //                                  |
  //                        +---------------------+
  //                        |   ClientUdpSocket   |
  //                        +---------------------+
  //                                  |
  //                                  +-------------------------------> Server
  //
  boost::asio::io_service         m_IoService;
  autotests::ClientUdpSocketPtr   m_pClientUdpSocket;
  network::ProtobufChannelPtr     m_pProtobufChannel;
  autotests::ProtobufSyncPipePtr  m_pSyncProtobufChannel;
  autotests::AccessPointClientPtr m_pClientAccessPoint;
  autotests::ClientCommutatorPtr  m_pRootClientCommutator;

};

} // namespace autotests

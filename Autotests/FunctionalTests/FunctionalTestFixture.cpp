#include "FunctionalTestFixture.h"
#include "Scenarios.h"

#include <yaml-cpp/yaml.h>

namespace autotests
{

FunctionalTestFixture::FunctionalTestFixture()
{
  Scenarios::m_pEnv = this;
}

void FunctionalTestFixture::SetUp()
{
  m_cfg = prephareConfiguration();
  m_application.initialize(m_cfg);

  YAML::Node worldState;
  if (initialWorldState(worldState))
    m_application.loadWorldState(worldState);

  m_application.start();

  m_clientAddress =
      boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 4455);
  m_serverLoginAddress =
      boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"),
        m_cfg.getLoginUdpPort());
  m_serverAddress =
      boost::asio::ip::udp::endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 0);

  m_pClientUdpSocket =
      std::make_shared<autotests::ClientUdpSocket>(m_IoService, m_clientAddress);
  m_pProtobufChannel     = std::make_shared<network::ProtobufChannel>();
  m_pSyncProtobufChannel = std::make_shared<autotests::ProtobufSyncPipe>();
  m_pClientAccessPoint   = std::make_shared<autotests::AccessPointClient>();
  m_pRootClientCommutator = std::make_shared<autotests::ClientCommutator>(0);

  m_pClientUdpSocket->setServerAddress(m_serverLoginAddress);
  m_pSyncProtobufChannel->setEnviromentProceeder([this](){ proceedEnviroment(10); });

  m_pClientUdpSocket->attachToTerminal(m_pProtobufChannel);
  m_pProtobufChannel->attachToChannel(m_pClientUdpSocket);
  m_pProtobufChannel->attachToTerminal(m_pSyncProtobufChannel);
  m_pSyncProtobufChannel->attachToChannel(m_pProtobufChannel);
  m_pClientAccessPoint->attachToSyncChannel(m_pSyncProtobufChannel);
  m_pRootClientCommutator->attachToSyncChannel(m_pSyncProtobufChannel);
}

void FunctionalTestFixture::TearDown()
{
  m_application.stop();

  m_pClientUdpSocket->detachFromTerminal();
  m_pProtobufChannel->detachFromChannel();
  m_pProtobufChannel->detachFromTerminal();
  m_pSyncProtobufChannel->detachFromTerminal();
  m_pClientAccessPoint->detachFromSyncChannel();
}

config::ApplicationCfg FunctionalTestFixture::prephareConfiguration()
{
  return config::ApplicationCfg()
      .setLoginUdpPort(6842)
      .setTotalThreads(1)
      .setPortsPool(
        config::PortsPoolCfg()
        .setBegin(25000)
        .setEnd(25100));
}

void FunctionalTestFixture::proceedEnviroment(uint32_t nMilliseconds)
{
  while(nMilliseconds--)
    m_application.proceedOnce(1000);

  while(m_IoService.poll());

  m_pProtobufChannel->handleBufferedMessages();
}



} // namespace autotests

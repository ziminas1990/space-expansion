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

  // Initializing client components
  m_pSocket         = std::make_shared<client::Socket>(m_IoService, m_clientAddress);
  m_pRootPipe       = std::make_shared<client::SyncPipe>();
  m_pAccessPanel    = std::make_shared<client::ClientAccessPanel>();
  m_pRootCommutator = std::make_shared<client::ClientCommutator>();

  m_pSocket->setServerAddress(m_serverLoginAddress);
  m_pRootPipe->setProceeder([this](){ proceedFreezedWorld(); });

  // Linking client components
  m_pSocket->attachToTerminal(m_pRootPipe);
  m_pRootPipe->attachToDownlevel(m_pSocket);
  m_pAccessPanel->attachToChannel(m_pRootPipe);
  m_pRootCommutator->attachToChannel(m_pRootPipe);

  // And, finaly, loading and running the world:
  m_application.initialize(m_cfg);
  YAML::Node worldState;
  if (initialWorldState(worldState))
    m_application.loadWorldState(worldState);
  m_application.start();
}

void FunctionalTestFixture::TearDown()
{
  m_application.stop();
  m_pRootPipe->detachDownlevel();
  m_pAccessPanel->detachChannel();
  m_pRootCommutator->detachChannel();
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

void FunctionalTestFixture::proceedFreezedWorld()
{
  // Time dependent logic should be almost freezed, only data exchange is allowed
  do {
    do {
      m_application.proceedOnce(0);
    } while(m_IoService.poll());
    std::this_thread::yield();
  } while(m_IoService.poll());
}

void FunctionalTestFixture::proceedEnviroment(uint32_t nMilliseconds, uint32_t nTickUs)
{
  proceedFreezedWorld();
  uint32_t nRemainUs = nMilliseconds * 1000;
  while(nRemainUs > nTickUs) {
    m_application.proceedOnce(nTickUs);
    nRemainUs -= nTickUs;
  };
  m_application.proceedOnce(nRemainUs);
  proceedFreezedWorld();
}



} // namespace autotests

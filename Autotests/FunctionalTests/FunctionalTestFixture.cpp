#include "FunctionalTestFixture.h"
#include "Scenarios.h"

#include <yaml-cpp/yaml.h>

namespace autotests
{

FunctionalTestFixture::FunctionalTestFixture()
  : m_lWorldFreezed(true)
{
  Scenarios::m_pEnv = this;
}

void FunctionalTestFixture::SetUp()
{
  m_cfg = prephareConfiguration();

  auto localHost = boost::asio::ip::address::from_string("127.0.0.1");

  m_clientAddress = boost::asio::ip::udp::endpoint(localHost, 4455);
  m_adminAddress  = boost::asio::ip::udp::endpoint(localHost, 4460);

  m_serverLoginAddress =
      boost::asio::ip::udp::endpoint(localHost, m_cfg.getLoginUdpPort());
  m_serverAddress = boost::asio::ip::udp::endpoint(localHost, 0);
  m_serverPrivilegedAddress =
      boost::asio::ip::udp::endpoint(localHost, m_cfg.getAdministratorCfg().getPort());

  // Initializing client components
  m_pSocket         = std::make_shared<client::PlayerSocket>(m_IoService, m_clientAddress);
  m_pRootPipe       = std::make_shared<client::PlayerPipe>();
  m_pAccessPanel    = std::make_shared<client::ClientAccessPanel>();
  m_pRootCommutator = std::make_shared<client::ClientCommutator>();

  m_pPrivilegedSocket = std::make_shared<client::PrivilegedSocket>(m_IoService,
                                                                   m_adminAddress);
  m_pPrivilegedPipe = std::make_shared<client::PrivilegedPipe>();
  m_pAdminPanel     = std::make_shared<client::AdministratorPanel>();

  m_pSocket->setServerAddress(m_serverLoginAddress);
  m_pPrivilegedSocket->setServerAddress(m_serverPrivilegedAddress);

  auto fProceeder = [this](){
    if (m_lWorldFreezed)
      proceedFreezedWorld();
    else
      proceedEnviroment(100, 1000);
  };

  m_pRootPipe->setProceeder(fProceeder);
  m_pPrivilegedPipe->setProceeder(fProceeder);

  // Linking client components
  m_pSocket->attachToTerminal(m_pRootPipe);
  m_pRootPipe->attachToDownlevel(m_pSocket);
  m_pAccessPanel->attachToChannel(m_pRootPipe);
  m_pRootCommutator->attachToChannel(m_pRootPipe);

  m_pPrivilegedPipe->attachToDownlevel(m_pPrivilegedSocket);
  m_pPrivilegedSocket->attachToTerminal(m_pPrivilegedPipe);
  m_pAdminPanel->attachToChannel(m_pPrivilegedPipe);

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
  std::cout << "Cycles = " << m_stat.nTotalCycles << ", time = " <<
               m_stat.nTotalTimeMs << " ms" << std::endl;
}

config::ApplicationCfg FunctionalTestFixture::prephareConfiguration()
{
  return config::ApplicationCfg()
      .setLoginUdpPort(6842)
      .setTotalThreads(1)
      .setPortsPool(
        config::PortsPoolCfg()
        .setBegin(25000)
        .setEnd(25100))
      .setAdministratorCfg(
        config::AdministratorCfg()
        .setPort(4372)
        .setLogin("god")
        .setPassord("god"));
}

void FunctionalTestFixture::proceedFreezedWorld()
{
  // Time dependent logic should be almost freezed, only data exchange is allowed
  do {
    do {
      m_application.proceedOnce(0);
      ++m_stat.nTotalCycles;
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
    ++m_stat.nTotalCycles;
  }
  m_stat.nTotalTimeMs += nMilliseconds;
  m_application.proceedOnce(nRemainUs);
  proceedFreezedWorld();
}



} // namespace autotests

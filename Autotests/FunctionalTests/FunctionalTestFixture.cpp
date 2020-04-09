#include "FunctionalTestFixture.h"
#include "Scenarios.h"

#include <yaml-cpp/yaml.h>
#include <Utils/Printers.h>

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

  auto fProceeder = [this](){ proceed(); };

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

  // Running application logic
  m_pMainThread = new std::thread([this]() { m_application.run(true); });

  // Logging admin panel (to control the time and etc)
  m_pAdminPanel->login(m_cfg.getAdministratorCfg().getLogin(),
                       m_cfg.getAdministratorCfg().getPassword(),
                       m_nAdminToken);
}

void FunctionalTestFixture::TearDown()
{
  m_application.getClock().terminate();
  m_application.nextCycle();
  if (m_pMainThread && m_pMainThread->joinable()) {
    m_pMainThread->join();
    delete m_pMainThread;
  }

  m_pRootPipe->detachDownlevel();
  m_pAccessPanel->detachChannel();
  m_pRootCommutator->detachChannel();
  m_pPrivilegedPipe->detachDownlevel();
  m_pAdminPanel->detachChannel();

  utils::ClockStat stat;
  m_application.getClock().exportStat(stat);

  std::cout << "Ticks = " << stat.nTicksCounter << ", real time = " <<
               utils::toTime(stat.nRealTimeUs) << ", ingame time = " <<
               utils::toTime(stat.nIngameTimeUs) << std::endl;

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

void FunctionalTestFixture::pauseTime()
{
  ASSERT_TRUE(m_application.getClock().switchToDebugMode());
}

void FunctionalTestFixture::resumeTime()
{
  ASSERT_TRUE(m_application.getClock().switchToRealtimeMode());
}

void FunctionalTestFixture::skipTime(uint32_t nTimeMs, uint32_t nTickUs)
{
  uint32_t nTimeUs = nTimeMs * 1000;
  pauseTime();
  m_application.getClock().setDebugTickUs(nTickUs);
  uint32_t ticks = nTimeUs / nTickUs;
  if (ticks * nTickUs < nTimeUs) {
    ++ticks;
  }
  ASSERT_TRUE(ticks * nTickUs >= nTimeUs);
  ASSERT_TRUE(ticks * nTickUs - nTimeUs < nTickUs);

  ASSERT_TRUE(m_application.getClock().proceedRequest(ticks));
  while (m_application.getClock().isDebugInProgress()) {
    proceed();
  }
}

void FunctionalTestFixture::proceed()
{
  do {
    m_application.nextCycle();
  } while(m_IoService.poll());
}

} // namespace autotests

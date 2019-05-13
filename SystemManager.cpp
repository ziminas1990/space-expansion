#include "SystemManager.h"
#include <thread>
#include "Conveyor/Proceeders.h"

SystemManager::~SystemManager()
{
  m_pAccessPanel->detachFromChannel();
  m_pLoginChannel->detachFromChannel();
  m_pLoginChannel->detachFromTerminal();
  delete m_pConveyor;
}

bool SystemManager::initialize(config::IApplicationCfg const& cfg)
{
  m_configuration = cfg;
  return createAllComponents()
      && configureComponents()
      && linkComponents();
}

bool SystemManager::start()
{
  for(size_t i = 1; i < m_configuration.getTotalThreads(); ++i)
    new std::thread([this]() { m_pConveyor->joinAsSlave();} );
  return true;
}

void SystemManager::stop()
{
  m_pConveyor->stop();
}

#ifdef AUTOTESTS_MODE
void SystemManager::proceedOnce(uint32_t nIntervalUs)
{
  m_pConveyor->proceed(nIntervalUs);
}
#endif

void SystemManager::proceed()
{
  conveyor::runRealTimeProceeder(m_pConveyor);
}

bool SystemManager::createAllComponents()
{
  m_pConveyor       = new conveyor::Conveyor(m_configuration.getTotalThreads());
  m_pUdpDispatcher  = std::make_shared<network::UdpDispatcher>(m_IoService);
  m_pLoginChannel   = std::make_shared<network::ProtobufChannel>();
  m_pAccessPanel    = std::make_shared<modules::AccessPanel>();
  m_pShipsManager   = std::make_shared<ships::ShipsManager>();
  m_pPlayersStorage = std::make_shared<world::PlayersStorage>();
  return true;
}

bool SystemManager::configureComponents()
{
  return true;
}

bool SystemManager::linkComponents()
{
  m_pUdpDispatcher->createUdpConnection(
        m_pLoginChannel, m_configuration.getLoginUdpPort());
  m_pLoginChannel->attachToTerminal(m_pAccessPanel);
  m_pAccessPanel->attachToPlayerStorage(m_pPlayersStorage);
  m_pAccessPanel->attachToConnectionManager(m_pUdpDispatcher);

  m_pPlayersStorage->attachToShipManager(m_pShipsManager);

  m_pConveyor->addLogicToChain(m_pUdpDispatcher);
  m_pConveyor->addLogicToChain(m_pShipsManager);

  return true;
}

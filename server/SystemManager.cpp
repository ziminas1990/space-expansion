#include "SystemManager.h"

#include <thread>
#include <iostream>
#include <iomanip>
#include <yaml-cpp/yaml.h>

#include <Privileged.pb.h>
#include "Modules/Commutator/CommutatorManager.h"
#include "Ships/ShipsManager.h"
#include "Conveyor/Proceeders.h"
#include "World/Resources.h"
#include <Arbitrators/ArbitratorsFactory.h>
#include <Utils/Printers.h>

//========================================================================================
// SystemManager
//========================================================================================

static utils::Clock* g_clock = nullptr;

SystemManager::~SystemManager()
{
  m_pAccessPanel->detachFromChannel();
  m_pLoginChannel->detachFromChannel();
  m_pLoginChannel->detachFromTerminal();
  m_pAdministratorPanel->detachFromChannel();
  m_pPrivilegedChannel->detachFromChannel();
  m_pPrivilegedChannel->detachFromTerminal();
  delete m_pConveyor;
}

bool SystemManager::initialize(const config::IApplicationCfg &cfg)
{
  m_configuration = cfg;
  return createAllComponents()
      && configureComponents()
      && linkComponents();
}

bool SystemManager::loadWorldState(YAML::Node const& data)
{
  world::Resource::initialize();

  YAML::Node const& blueprintsSection = data["Blueprints"];
  assert(blueprintsSection.IsDefined());
  if (!blueprintsSection.IsDefined())
    return false;

  {
    YAML::Node const& modulesBlueprintsSection = blueprintsSection["Modules"];
    if (!m_blueprints.loadModulesBlueprints(modulesBlueprintsSection)) {
      assert("Fail to load modules blueprints section!" == nullptr);
      return false;
    }
  }
  {
    YAML::Node const& shipsBlueprintsSection = blueprintsSection["Ships"];
    if (!m_blueprints.loadShipsBlueprints(shipsBlueprintsSection)) {
      assert("Fail to load ships blueprints section!" == nullptr);
      return false;
    }
  }

  YAML::Node const& playersState = data["Players"];
  if (!m_pPlayersStorage->loadState(playersState, m_blueprints)) {
    assert(false);
    return false;
  }

  YAML::Node const& worldState = data["World"];
  if (worldState.IsDefined() && !m_world.loadState(worldState)) {
    assert(false);
    return false;
  }

  YAML::Node const& arbitratorCfg = data["Arbitrator"];
  if (arbitratorCfg.IsDefined()) {
    m_pArbitrator = arbitrator::Factory::make(arbitratorCfg, m_pPlayersStorage);
    if (!m_pArbitrator) {
      assert("Failed to load arbitrator" == nullptr);
      return false;
    }
    m_pConveyor->addLogicToChain(m_pArbitrator);
  }
  return true;
}

#ifndef AUTOTESTS_MODE
void SystemManager::run(bool lColdStart)
{
  assert(g_clock == nullptr && "Another system manage is running?");
  g_clock = &m_clock;
  const uint32_t nMinTickLengthUs = 100;

  startConveyor();
  m_clock.start(lColdStart);

  uint64_t nOneSecondsTimeout  = 1000000;

  printStatisticHeader();

  while (!m_clock.isTerminated()) {
    uint32_t nIntervalUs = m_clock.getNextInterval();
    m_pConveyor->proceed(nIntervalUs);
    if (nIntervalUs < nMinTickLengthUs) {
      std::this_thread::yield();
    }

    if (nOneSecondsTimeout < nIntervalUs) {
      nOneSecondsTimeout += 1000000;
      printStatistic();
    }
    nOneSecondsTimeout -= nIntervalUs;
  }

  stopConveyor();
  g_clock = nullptr;
}

void SystemManager::nextCycle()
{
  assert("This function can be used only from autotests" == nullptr);
}

uint64_t SystemManager::getIngameTime()
{
  assert(g_clock && "System manager is not running!");
  return g_clock->now();
}

#else // #ifndef AUTOTESTS_MODE
void SystemManager::run(bool lColdStart)
{
  assert(g_clock == nullptr && "Another system manage is running?");
  g_clock = &m_clock;
  startConveyor();
  m_clock.start(lColdStart);

  while (!m_clock.isTerminated()) {
    m_barrier.wait();
    uint32_t nIntervalUs = m_clock.getNextInterval();
    m_pConveyor->proceed(nIntervalUs);
    m_barrier.wait();
  }

  stopConveyor();
  g_clock = nullptr;
}

void SystemManager::nextCycle()
{
  m_barrier.wait();
  m_barrier.wait();
}

uint64_t SystemManager::getIngameTime()
{
  return g_clock ? g_clock->now() : 0;
}
#endif // #ifndef AUTOTESTS_MODE

bool SystemManager::createAllComponents()
{
  m_pConveyor = new conveyor::Conveyor(m_configuration.getTotalThreads());

  m_pNewtonEngine             = std::make_shared<newton::NewtonEngine>();
  m_pFilteringManager         = std::make_shared<tools::ObjectsFilteringManager>();
  m_pShipsManager             = std::make_shared<ships::ShipsManager>();
  m_pCommutatorsManager       = std::make_shared<modules::CommutatorManager>();
  m_pEnginesManager           = std::make_shared<modules::EngineManager>();
  m_pCelestialScannerManager  = std::make_shared<modules::CelestialScannerManager>();
  m_pAsteroidScannerManager   = std::make_shared<modules::AsteroidScannerManager>();
  m_pResourceContainerManager = std::make_shared<modules::ResourceContainerManager>();
  m_pAsteroidMinerManager     = std::make_shared<modules::AsteroidMinerManager>();
  m_pBlueprintsStorageManager = std::make_shared<modules::BlueprintsStorageManager>();
  m_pShipyardManager          = std::make_shared<modules::ShipyardManager>();
  m_pSystemClockManager       = std::make_shared<modules::SystemClockManager>();

  m_pUdpDispatcher  =
      std::make_shared<network::UdpDispatcher>(
        m_IoService,
        m_configuration.getPortsPoolcfg().begin(),
        m_configuration.getPortsPoolcfg().end());
  m_pLoginChannel       = std::make_shared<network::PlayerChannel>();
  m_pAccessPanel        = std::make_shared<modules::AccessPanel>();

  if (m_configuration.getAdministratorCfg().isValid()) {
    m_pPrivilegedChannel  = std::make_shared<network::PrivilegedChannel>();
    m_pAdministratorPanel = std::make_shared<AdministratorPanel>(
          m_configuration.getAdministratorCfg(),
          std::time(nullptr));
  }

  m_pPlayersStorage     = std::make_shared<world::PlayersStorage>();
  return true;
}

bool SystemManager::configureComponents()
{
  return true;
}

bool SystemManager::linkComponents()
{
  if (m_pPrivilegedChannel && m_pAdministratorPanel) {
    m_pPrivilegedSocket = m_pUdpDispatcher->createUdpConnection(
          m_configuration.getAdministratorCfg().getPort());
    m_pPrivilegedSocket->attachToTerminal(m_pPrivilegedChannel);
    m_pPrivilegedChannel->attachToChannel(m_pPrivilegedSocket);
    m_pPrivilegedChannel->attachToTerminal(m_pAdministratorPanel);
    m_pAdministratorPanel->attachToChannel(m_pPrivilegedChannel);
    m_pAdministratorPanel->attachToSystemManager(this);
  }

  m_pLoginSocket = m_pUdpDispatcher->createUdpConnection(
        m_configuration.getLoginUdpPort());
  m_pLoginSocket->attachToTerminal(m_pLoginChannel);
  m_pLoginChannel->attachToChannel(m_pLoginSocket);
  m_pLoginChannel->attachToTerminal(m_pAccessPanel);
  m_pAccessPanel->attachToChannel(m_pLoginChannel);
  m_pAccessPanel->attachToPlayerStorage(m_pPlayersStorage);
  m_pAccessPanel->attachToConnectionManager(m_pUdpDispatcher);

  m_pConveyor->addLogicToChain(m_pUdpDispatcher);
  m_pConveyor->addLogicToChain(m_pAccessPanel);
  if (m_pAdministratorPanel) {
    m_pConveyor->addLogicToChain(m_pAdministratorPanel);
  }
  m_pConveyor->addLogicToChain(m_pNewtonEngine);
  m_pConveyor->addLogicToChain(m_pFilteringManager);
  m_pConveyor->addLogicToChain(m_pCommutatorsManager);
  m_pConveyor->addLogicToChain(m_pSystemClockManager);
  m_pConveyor->addLogicToChain(m_pShipsManager);
  m_pConveyor->addLogicToChain(m_pEnginesManager);
  m_pConveyor->addLogicToChain(m_pCelestialScannerManager);
  m_pConveyor->addLogicToChain(m_pAsteroidScannerManager);
  m_pConveyor->addLogicToChain(m_pResourceContainerManager);
  m_pConveyor->addLogicToChain(m_pAsteroidMinerManager);
  m_pConveyor->addLogicToChain(m_pBlueprintsStorageManager);
  m_pConveyor->addLogicToChain(m_pShipyardManager);
  return true;
}

void SystemManager::startConveyor()
{
  for(size_t i = 1; i < m_configuration.getTotalThreads(); ++i) {
    m_slaves.push_back(new std::thread([this]() { m_pConveyor->joinAsSlave();} ));
  }
}

void SystemManager::stopConveyor()
{
  m_pConveyor->stop();
  for (std::thread* pSlaveThread : m_slaves) {
    if (pSlaveThread->joinable()) {
      pSlaveThread->join();
    }
    delete pSlaveThread;
  }
  m_slaves.clear();
}

void SystemManager::printStatisticHeader()
{
  std::cout << std::right << std::setw(12) << "Ticks Total"
            << std::right << std::setw(17) << "Real Time"
            << std::right << std::setw(17) << "Ingame Time"
            << std::right << std::setw(17) << "Deviation"
            << std::right << std::setw(12) << "Avg. Tick"
            << std::endl;
}

void SystemManager::printStatistic()
{
  utils::ClockStat stat;
  m_clock.exportStat(stat);
  std::cout << std::right << std::setw(12) << stat.nTicksCounter
            << std::right << std::setw(17) << utils::toTime(stat.nRealTimeUs)
            << std::right << std::setw(17) << utils::toTime(stat.nIngameTimeUs)
            << std::right << std::setw(17) << utils::toTime(stat.nDeviationUs)
            << std::right << std::setw(12) << utils::toTime(stat.nAvgTickDurationPerPeriod)
            << std::endl;
}

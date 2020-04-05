#include "SystemManager.h"

#include <thread>
#include <yaml-cpp/yaml.h>
#include <iostream>

#include <Priveledeged.pb.h>
#include "Modules/Commutator/CommutatorManager.h"
#include "Ships/ShipsManager.h"
#include "Conveyor/Proceeders.h"
#include "World/Resources.h"
#include <Arbitrators/ArbitratorsFactory.h>

//========================================================================================
// SystemManager
//========================================================================================

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

bool SystemManager::start()
{
  for(size_t i = 1; i < m_configuration.getTotalThreads(); ++i) {
    m_slaves.push_back(new std::thread([this]() { m_pConveyor->joinAsSlave();} ));
  }
  return true;
}

bool SystemManager::freeze()
{
  switch (m_eClockState) {
    case ClockState::eManualMode: {
      m_proceedSteps.m_nTicksLeft = 0;
      // nobreak
    }
    case ClockState::eRunInRealTime: {
      m_eClockState = ClockState::eFreezed;
      // nobreak
    }
    case ClockState::eFreezed: {
      return true;
    }
    default:
      return false;
  }
}

void SystemManager::resume()
{
  assert(m_eClockState == ClockState::eFreezed);
  if (m_eClockState == ClockState::eFreezed) {
    m_eClockState = ClockState::eRunInRealTime;
  }
}

bool SystemManager::proceedInterval(uint32_t nTickUs, uint32_t nInterval)
{
  if (m_eClockState != ClockState::eFreezed) {
    return false;
  }
  m_eClockState                    = ClockState::eManualMode;
  m_proceedSteps.m_nTickDurationUs = nTickUs;
  m_proceedSteps.m_nTicksLeft      = nInterval / nTickUs;
  return true;
}

bool SystemManager::stop()
{
  assert(m_eClockState != ClockState::eManualMode);
  if (m_eClockState == ClockState::eManualMode) {
    return false;
  }
  m_eClockState = ClockState::eTerminate;
  return true;
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

void SystemManager::proceedOnce(uint32_t nIntervalUs)
{
  m_pConveyor->proceed(nIntervalUs);
}

void SystemManager::exportStat(SystemManager::Stats &out) const
{
  out.m_inGameTime  = m_inGameTime;
  out.m_freezedTime = m_freezedTime;
}

void SystemManager::proceed()
{
  const auto nMinTickSize = std::chrono::microseconds(100);
  const auto nMaxTickSize = std::chrono::milliseconds(20);
  auto       startTime    = std::chrono::high_resolution_clock::now();

  while (m_eClockState != ClockState::eTerminate)
  {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::microseconds>(
          currentTime - (startTime + m_inGameTime + m_freezedTime));

    switch (m_eClockState) {
      case ClockState::eRunInRealTime: {
        if (dt < nMinTickSize) {
          std::this_thread::yield();
          continue;
        }

        if (dt > nMaxTickSize) {
          // Ooops, seems there is a perfomance problem
          m_freezedTime += dt - nMaxTickSize;
#ifdef DEBUG_MODE
          std::cout << "Freezed for " << (dt - nMaxTickSize).count() <<
                       " usec..." << std::endl;
#endif // ifdef DEBUG_MODE
          dt = nMaxTickSize;
        }
        break;
      }
      case ClockState::eFreezed: {
        m_freezedTime += dt;
        dt = std::chrono::microseconds(0);
        break;
      }
      case ClockState::eManualMode:
        assert(m_proceedSteps.m_nTicksLeft > 0);
        assert(m_proceedSteps.m_nTickDurationUs > 0);

        m_freezedTime += dt;
        dt = std::chrono::microseconds(m_proceedSteps.m_nTickDurationUs);
        m_freezedTime -= dt;
        if (--m_proceedSteps.m_nTicksLeft == 0) {
          m_eClockState = ClockState::eFreezed;
        }
        break;
      default: {
        assert("Unexpected state!" == nullptr);
      }
    }

    m_pConveyor->proceed(static_cast<uint32_t>(dt.count()));
    m_inGameTime += dt;
  }

  stopConveyor();
}

bool SystemManager::createAllComponents()
{
  m_pConveyor = new conveyor::Conveyor(m_configuration.getTotalThreads());

  m_pNewtonEngine             = std::make_shared<newton::NewtonEngine>();
  m_pShipsManager             = std::make_shared<ships::ShipsManager>();
  m_pCommutatorsManager       = std::make_shared<modules::CommutatorManager>();
  m_pEnginesManager           = std::make_shared<modules::EngineManager>();
  m_pCelestialScannerManager  = std::make_shared<modules::CelestialScannerManager>();
  m_pAsteroidScannerManager   = std::make_shared<modules::AsteroidScannerManager>();
  m_pResourceContainerManager = std::make_shared<modules::ResourceContainerManager>();
  m_pAsteroidMinerManager     = std::make_shared<modules::AsteroidMinerManager>();
  m_pBlueprintsStorageManager = std::make_shared<modules::BlueprintsStorageManager>();
  m_pShipyardManager          = std::make_shared<modules::ShipyardManager>();

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
    m_pAdministratorPanel->attachToSystemManager(this);
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
    m_pUdpDispatcher->createUdpConnection(
          m_pPrivilegedChannel,
          m_configuration.getAdministratorCfg().getPort());
    m_pPrivilegedChannel->attachToTerminal(m_pAdministratorPanel);
    m_pAdministratorPanel->attachToChannel(m_pPrivilegedChannel);
  }

  m_pUdpDispatcher->createUdpConnection(m_pLoginChannel,
                                        m_configuration.getLoginUdpPort());
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
  m_pConveyor->addLogicToChain(m_pCommutatorsManager);
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

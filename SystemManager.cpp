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
  for(size_t i = 1; i < m_configuration.getTotalThreads(); ++i)
    new std::thread([this]() { m_pConveyor->joinAsSlave();} );
  return true;
}

void SystemManager::stop()
{
  m_pConveyor->stop();
}

void SystemManager::proceedOnce(uint32_t nIntervalUs)
{
  m_pConveyor->proceed(nIntervalUs);
}

void SystemManager::proceed()
{
  const auto nMinTickSize = std::chrono::microseconds(100);
  const auto nMaxTickSize = std::chrono::milliseconds(20);
  auto startTime    = std::chrono::high_resolution_clock::now();
  while (true)
  {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto dt =
        std::chrono::duration_cast<std::chrono::microseconds>(
          currentTime - startTime - m_inGameTime);
    if (dt < nMinTickSize) {
      std::this_thread::yield();
      continue;
    }
    if (dt > nMaxTickSize)
      dt = nMaxTickSize;

#ifdef DEBUG_MODE
    if (dt > nMaxTickSize / 2) {
      std::cout << "Proceeding " << dt.count() << " usec..." << std::endl;
    }
#endif // ifdef DEBUG_MODE

    m_pConveyor->proceed(static_cast<uint32_t>(dt.count()));
    m_inGameTime += dt;
  }
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

#pragma once

#include <boost/asio.hpp>
#include <Utils/YamlForwardDeclarations.h>
#include <Utils/RandomSequence.h>

#include "ConfigDI/Containers.h"
#include "Conveyor/Conveyor.h"
#include "Network/UdpDispatcher.h"
#include "Network/ProtobufChannel.h"
#include "Newton/NewtonEngine.h"
#include "World/PlayersStorage.h"
#include <World/Grid.h>
#include <World/World.h>
#include "Blueprints/BlueprintsLibrary.h"
#include <Utils/Clock.h>

#include "Newton/NewtonEngine.h"
#include <AdministratorPanel/AdministratorPanel.h>
#include <Modules/Fwd.h>
#include <Arbitrators/BaseArbitrator.h>
#include <ConveyorTools/ObjectsFilter.h>

class SystemManager
{
public:
  SystemManager(uint32_t seed);
  ~SystemManager();

  bool initialize(config::IApplicationCfg const& cfg);
  bool loadWorldState(YAML::Node const& data);

  void run(bool lDebugMode = false);

  // for functional tests only:
  void nextCycle();

  utils::Clock& getClock() { return m_clock; }

  world::World& getWorld() { return m_world; }

  tools::ObjectsFilteringManagerPtr getFilteringManager() const {
    return m_pFilteringManager;
  }

private:
  bool createAllComponents();
  bool configureComponents();
  bool linkComponents();

  void startConveyor();
  void stopConveyor();

  static void printStatisticHeader();
  void printStatistic();

private:
  config::ApplicationCfg       m_configuration;
  utils::Clock                 m_clock;
  conveyor::Conveyor*          m_pConveyor = nullptr;
  std::vector<std::thread*>    m_slaves;
  boost::asio::io_service      m_IoService;
  utils::RandomSequence        m_randomizer;

#ifdef AUTOTESTS_MODE
  boost::fibers::barrier m_barrier = boost::fibers::barrier(2);
    // This barrier will be used to synchronize application cycles with
    // functional tests cycles
#endif

  blueprints::BlueprintsLibrary m_blueprints;
    // Blueprints of modules, that are avaliable for all players right from
    // the start of the game

  // Managers for all logics
  newton::NewtonEnginePtr              m_pNewtonEngine;
  tools::ObjectsFilteringManagerPtr    m_pFilteringManager;
  modules::ShipManagerPtr              m_pShipsManager;
  modules::CommutatorManagerPtr        m_pCommutatorsManager;
  modules::EngineManagerPtr            m_pEnginesManager;
  modules::CelestialScannerManagerPtr  m_pCelestialScannerManager;
  modules::PassiveScannerManagerPtr    m_pPassiveScannerManager;
  modules::AsteroidScannerManagerPtr   m_pAsteroidScannerManager;
  modules::ResourceContainerManagerPtr m_pResourceContainerManager;
  modules::AsteroidMinerManagerPtr     m_pAsteroidMinerManager;
  modules::BlueprintsStorageManagerPtr m_pBlueprintsStorageManager;
  modules::ShipyardManagerPtr          m_pShipyardManager;
  modules::SystemClockManagerPtr       m_pSystemClockManager;

  // Receiving stacks
  network::UdpDispatcherPtr     m_pUdpDispatcher;

  network::UdpSocketPtr         m_pLoginSocket;
  network::PlayerChannelPtr     m_pLoginChannel;
  modules::AccessPanelPtr       m_pAccessPanel;

  network::UdpSocketPtr         m_pPrivilegedSocket;
  network::PrivilegedChannelPtr m_pPrivilegedChannel;
  AdministratorPanelPtr         m_pAdministratorPanel;

  // World
  world::Grid             m_globalGrid;
  world::World            m_world;
  world::PlayerStoragePtr m_pPlayersStorage;

  arbitrator::BaseArbitratorPtr m_pArbitrator;
};

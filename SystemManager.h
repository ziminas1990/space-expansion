#pragma once

#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include "Utils/YamlForwardDeclarations.h"

#include "ConfigDI/Containers.h"
#include "Conveyor/Conveyor.h"
#include "Network/UdpDispatcher.h"
#include "Network/ProtobufChannel.h"
#include "Modules/AccessPanel/AccessPanel.h"
#include "Newton/NewtonEngine.h"
#include "World/PlayersStorage.h"
#include <World/World.h>
#include "Blueprints/BlueprintsLibrary.h"
#include <AdministratorPanel/AdministratorPanel.h>

#include "Newton/NewtonEngine.h"
#include "Ships/ShipsManager.h"
#include "Modules/Commutator/CommutatorManager.h"
#include "Modules/Engine/EnginesManager.h"
#include "Modules/CelestialScanner/CelestialScannerManager.h"
#include "Modules/AsteroidScanner/AsteroidScannerManager.h"
#include "Modules/ResourceContainer/ResourceContainerManager.h"
#include "Modules/AsteroidMiner/AsteroidMinerManager.h"
#include "Modules/BlueprintsStorage/BlueprintsStorageManager.h"
#include "Modules/Shipyard/ShipyardManager.h"
#include "Arbitrators/BaseArbitrator.h"

class SystemManager
{
public:
  enum class ClockState {
    eRunInRealTime,
      // Game logic runs in real time mode
    eFreezed,
      // Game logic is freezed
    eManualMode,
      // Game logic is proceeded in manual mode
    eTerminate
      // Game logic is stopped
  };

  struct Stats {
    std::chrono::microseconds m_inGameTime;
      // How much time has passed in the game's world scince it was run
    std::chrono::microseconds m_freezedTime;
      // How much time the world has been freezed
  };

public:
  ~SystemManager();

  bool initialize(config::IApplicationCfg const& cfg);
  bool loadWorldState(YAML::Node const& data);

  bool start();
  bool freeze();
  void resume();
  bool proceedInterval(uint32_t nTickUs, uint32_t nTotalTicks);
  bool stop();
  void stopConveyor();

  ClockState getClockState() const { return m_eClockState; }
  std::chrono::microseconds now() const { return m_inGameTime; }

  void proceed();
  void proceedOnce(uint32_t nIntervalUs);

  void exportStat(Stats& out) const;

private:
  bool createAllComponents();
  bool configureComponents();
  bool linkComponents();

private:
  config::ApplicationCfg       m_configuration;
  conveyor::Conveyor*          m_pConveyor = nullptr;
  std::vector<std::thread*>    m_slaves;
  ClockState                   m_eClockState = ClockState::eFreezed;

  boost::asio::io_service      m_IoService;

  struct {
    uint32_t m_nTickDurationUs = 0;
    uint32_t m_nTicksLeft      = 0;
  } m_proceedSteps;
    // This struct is used, when SystemManager is in freezed state and the time
    // is controlled manually (from adminPanel or integration tests)

  blueprints::BlueprintsLibrary m_blueprints;
    // Blueprints of modules, that are avaliable for all players right from
    // the start of the game

  // Managers for all logics
  newton::NewtonEnginePtr              m_pNewtonEngine;
  ships::ShipsManagerPtr               m_pShipsManager;
  modules::CommutatorManagerPtr        m_pCommutatorsManager;
  modules::EngineManagerPtr            m_pEnginesManager;
  modules::CelestialScannerManagerPtr  m_pCelestialScannerManager;
  modules::AsteroidScannerManagerPtr   m_pAsteroidScannerManager;
  modules::ResourceContainerManagerPtr m_pResourceContainerManager;
  modules::AsteroidMinerManagerPtr     m_pAsteroidMinerManager;
  modules::BlueprintsStorageManagerPtr m_pBlueprintsStorageManager;
  modules::ShipyardManagerPtr          m_pShipyardManager;

  network::UdpDispatcherPtr     m_pUdpDispatcher;
  network::PlayerChannelPtr     m_pLoginChannel;
  modules::AccessPanelPtr       m_pAccessPanel;
  network::PrivilegedChannelPtr m_pPrivilegedChannel;
  AdministratorPanelPtr         m_pAdministratorPanel;

  world::World            m_world;
  world::PlayerStoragePtr m_pPlayersStorage;

  arbitrator::BaseArbitratorPtr m_pArbitrator;
};

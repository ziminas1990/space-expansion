#pragma once

#include <boost/asio.hpp>
#include "Utils/YamlForwardDeclarations.h"

#include "ConfigDI/Containers.h"
#include "Conveyor/Conveyor.h"
#include "Network/UdpDispatcher.h"
#include "Network/ProtobufChannel.h"
#include "Modules/AccessPanel/AccessPanel.h"
#include "Newton/NewtonEngine.h"
#include "World/PlayersStorage.h"
#include <World/World.h>
#include "Blueprints/Modules/BlueprintsLibrary.h"
#include "Blueprints/Ships/ShipBlueprintsLibrary.h"

#include "Newton/NewtonEngine.h"
#include "Ships/ShipsManager.h"
#include "Modules/Commutator/CommutatorManager.h"
#include "Modules/Engine/EnginesManager.h"
#include "Modules/CelestialScanner/CelestialScannerManager.h"
#include "Modules/AsteroidScanner/AsteroidScannerManager.h"
#include "Modules/ResourceContainer/ResourceContainerManager.h"
#include "Modules/AsteroidMiner/AsteroidMinerManager.h"

class SystemManager
{
public:

  ~SystemManager();

  bool initialize(config::IApplicationCfg const& cfg);
  bool loadWorldState(YAML::Node const& data);

  bool start();
  void stop();

  [[noreturn]] void proceed();

  void proceedOnce(uint32_t nIntervalUs);

private:
  bool createAllComponents();
  bool configureComponents();
  bool linkComponents();

private:
  config::ApplicationCfg       m_configuration;
  conveyor::Conveyor*          m_pConveyor = nullptr;
  boost::asio::io_service      m_IoService;

  modules::BlueprintsLibrary   m_modulesBlueprints;
    // Blueprints of modules, that a avaliable for all players right from the start of
    // the game
  ships::ShipBlueprintsLibrary m_shipsBlueprints;
    // Blueprints of ships, that a avaliable for all players  right from the start of the
    // game

  // Managers for all logics
  newton::NewtonEnginePtr       m_pNewtonEngine;
  ships::ShipsManagerPtr        m_pShipsManager;
  modules::CommutatorManagerPtr m_pCommutatorsManager;
  modules::EngineManagerPtr     m_pEnginesManager;
  modules::CelestialScannerManagerPtr  m_pCelestialScannerManager;
  modules::AsteroidScannerManagerPtr   m_pAsteroidScannerManager;
  modules::ResourceContainerManagerPtr m_pResourceContainerManager;
  modules::AsteroidMinerManagerPtr     m_pAsteroidMinerManager;

  network::UdpDispatcherPtr   m_pUdpDispatcher;
  network::ProtobufChannelPtr m_pLoginChannel;

  modules::AccessPanelPtr     m_pAccessPanel;

  world::World            m_world;
  world::PlayerStoragePtr m_pPlayersStorage;
};

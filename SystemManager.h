#pragma once

#include <boost/asio.hpp>
#include "ConfigDI/Containers.h"
#include "Conveyor/Conveyor.h"
#include "Network/UdpDispatcher.h"
#include "Network/ProtobufChannel.h"
#include "Modules/AccessPanel/AccessPanel.h"
#include "Ships/ShipsManager.h"
#include "World/PlayersStorage.h"

class SystemManager
{
public:

  ~SystemManager();

  bool initialize(config::IApplicationCfg const& cfg);
  bool start();
  void stop();

  [[noreturn]] void proceed();

#ifdef AUTOTESTS_MODE
  void proceedOnce(uint32_t nIntervalUs);
#endif

private:
  bool createAllComponents();
  bool configureComponents();
  bool linkComponents();

private:
  config::ApplicationCfg      m_configuration;
  conveyor::Conveyor*         m_pConveyor;
  boost::asio::io_service     m_IoService;

  network::UdpDispatcherPtr   m_pUdpDispatcher;
  network::ProtobufChannelPtr m_pLoginChannel;

  modules::AccessPanelPtr     m_pAccessPanel;

  ships::ShipsManagerPtr      m_pShipsManager;
  world::PlayerStoragePtr     m_pPlayersStorage;
};

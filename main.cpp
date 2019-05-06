#ifndef AUTOTESTS_MODE

#include <thread>

#include <boost/asio.hpp>

#include "Conveyor/Conveyor.h"
#include "Conveyor/Proceeders.h"

#include "Network/BufferedTerminal.h"
#include "Network/UdpDispatcher.h"

#include <World/PlayersStorage.h>

#include "Modules/AccessPanel/AccessPanel.h"
#include "Modules/CommandCenter/CommanCenterManager.h"

[[noreturn]] int main(int, char*[])
{
  uint16_t nTotalThreadsCount = 4;

  boost::asio::io_service ioContext;


  // Creating components
  network::ConnectionManagerPtr pConnectionManager =
      std::make_shared<network::UdpDispatcher>(ioContext);

  modules::CommandCenterManagerPtr pCommandCenterManager =
      std::make_shared<modules::CommandCenterManager>();

  world::PlayerStoragePtr pPlayersStorage =
      std::make_shared<world::PlayerStorage>();

  modules::AccessPanelPtr pAccessPanel = std::make_shared<modules::AccessPanel>();

  // Setting and linking components
  pPlayersStorage->attachToCommandCenterManager(pCommandCenterManager);
  pAccessPanel->attachToPlayerStorage(pPlayersStorage);
  pAccessPanel->attachToConnectionManager(pConnectionManager);
  pConnectionManager->createUdpConnection(pAccessPanel, 31415);

  // Creating and running conveoyr
  conveyor::Conveyor conveyor(nTotalThreadsCount);
  conveyor.addLogicToChain(pConnectionManager);
  conveyor.addLogicToChain(pCommandCenterManager);

  for(size_t i = 1; i < nTotalThreadsCount; ++i)
    new std::thread([&conveyor]() { conveyor.joinAsSlave();} );

  // Main application loop starts here:
  conveyor::runRealTimeProceeder(&conveyor);
}

#else

#include <gtest/gtest.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#endif

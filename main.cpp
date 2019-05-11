#ifndef AUTOTESTS_MODE

#include <thread>

#include <boost/asio.hpp>

#include "Conveyor/Conveyor.h"
#include "Conveyor/Proceeders.h"

#include "Network/ProtobufChannel.h"
#include "Network/BufferedTerminal.h"
#include "Network/UdpDispatcher.h"

#include <World/PlayersStorage.h>

#include "Modules/AccessPanel/AccessPanel.h"
#include "Ships/ShipsManager.h"

[[noreturn]] int main(int, char*[])
{
  uint16_t nTotalThreadsCount = 4;

  boost::asio::io_service ioContext;

  // Creating components
  network::ConnectionManagerPtr pConnectionManager =
      std::make_shared<network::UdpDispatcher>(ioContext);

  ships::ShipsManagerPtr pShipsManager = std::make_shared<ships::ShipsManager>();

  world::PlayerStoragePtr pPlayersStorage =
      std::make_shared<world::PlayersStorage>();

  network::ProtobufChannelPtr pLoginChannel =
      std::make_shared<network::ProtobufChannel>();
  modules::AccessPanelPtr pAccessPanel = std::make_shared<modules::AccessPanel>();

  // Setting and linking components
  pPlayersStorage->attachToShipManager(pShipsManager);
  pAccessPanel->attachToPlayerStorage(pPlayersStorage);
  pAccessPanel->attachToConnectionManager(pConnectionManager);
  pConnectionManager->createUdpConnection(pLoginChannel, 31415);
  pLoginChannel->attachToTerminal(pAccessPanel);

  // Creating and running conveoyr
  conveyor::Conveyor conveyor(nTotalThreadsCount);
  conveyor.addLogicToChain(pConnectionManager);
  conveyor.addLogicToChain(pShipsManager);

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

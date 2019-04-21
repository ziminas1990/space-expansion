#include <thread>
#include <chrono>

#include <boost/asio.hpp>

#include "Conveyor/Conveyor.h"

#include "Network/BufferedTerminal.h"
#include "Network/ConnectionManager.h"
#include "Network/TcpListener.h"

#include "Modules/AccessPanel/AccessPanel.h"

[[noreturn]] int main(int, char*[])
{
  uint16_t nTotalThreadsCount = 4;

  boost::asio::io_service ioContext;


  // Creating components
  network::ConnectionManagerPtr pConnectionManager =
      std::make_shared<network::ConnectionManager>(ioContext);

  network::TcpListener tcpListener(ioContext, 31419);

  modules::AccessPanelFacotryPtr pAccessPanelFactory =
      std::make_shared<modules::AccessPanelFacotry>();

  // Setting and linking components
  tcpListener.attachToConnectionManage(pConnectionManager);
  pAccessPanelFactory->setCreationData(pConnectionManager);
  tcpListener.attachToTeminalFactory(pAccessPanelFactory);

  // Creating and running conveoyr
  conveyor::Conveyor conveyor(nTotalThreadsCount);
  conveyor.addLogicToChain(pConnectionManager);

  for(size_t i = 1; i < nTotalThreadsCount; ++i)
    new std::thread([&conveyor]() { conveyor.joinAsSlave();} );

  tcpListener.start();

  // Main application loop starts here:
  auto nMinTickSize = std::chrono::microseconds(100);
  auto nMaxTickSize = std::chrono::milliseconds(20);
  auto startTime    = std::chrono::high_resolution_clock::now();
  auto inGameTime   = std::chrono::microseconds(0);
  while (true)
  {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto dt =
        std::chrono::duration_cast<std::chrono::microseconds>(
          currentTime - startTime - inGameTime);
    if (dt < nMinTickSize) {
      std::this_thread::yield();
      continue;
    }
    if (dt > nMaxTickSize)
      dt = nMaxTickSize;
    if (dt > nMinTickSize * 10)
      std::cout << "Proceeding " << dt.count() << " usec..." << std::endl;
    conveyor.proceed(static_cast<size_t>(dt.count()));
    inGameTime += dt;
  }
}

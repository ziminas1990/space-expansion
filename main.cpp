#include <thread>
#include "Conveyor/Conveyor.h"

//#include "Network/UdpServer.h"

int main(int, char*[])
{
  uint16_t nTotalThreadsCount = 4;
  conveyor::Conveyor conveyor(nTotalThreadsCount);

//  network::UdpServerPtr pUdpServer = std::make_shared<network::UdpServer>();

//  for(uint16_t nPort = 5000; nPort < 5100; nPort++)
//    pUdpServer->addHandlerOnPort(nPort);

//  conveyor.addLogicToChain(std::move(pUdpServer));

  for(size_t i = 1; i < nTotalThreadsCount; ++i)
    new std::thread([&conveyor]() { conveyor.joinAsSlave();} );

  for (size_t tick = 0; tick < 3000; ++tick) {
    conveyor.proceed(tick);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return 0;
}

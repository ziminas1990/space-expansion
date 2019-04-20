#include <thread>
#include <chrono>

#include <boost/asio.hpp>

#include "Conveyor/Conveyor.h"

#include "Network/BufferedTerminal.h"
#include "Network/ConnectionManager.h"
#include "Network/TcpListener.h"


class EchoTerminal : public network::BufferedTerminal
{
protected:
  // overrides BufferedTerminal interface
  void handleMessage(network::MessagePtr pMessage, size_t nLength) override
  {
    send(pMessage, nLength);
  }
};

class EchoTerminalFactory : public network::IBufferedTerminalFactory
{
public:
  // overrides from IBufferedTerminalFactory interface
  network::BufferedTerminalPtr make() { return std::make_shared<EchoTerminal>(); }
};

using EchoTerminalFactoryPtr     = std::shared_ptr<EchoTerminalFactory>;
using EchoTerminalFactoryWeakPtr = std::weak_ptr<EchoTerminalFactory>;

[[noreturn]] int main(int, char*[])
{
  uint16_t nTotalThreadsCount = 4;

  boost::asio::io_service ioContext;

  network::ConnectionManagerPtr pConnectionManager =
      std::make_shared<network::ConnectionManager>(ioContext);

  network::TcpListener tcpListener(ioContext, 31419);
  tcpListener.attachToConnectionManage(pConnectionManager);

  EchoTerminalFactoryPtr pTcpEchoTerminalFactory =
      std::make_shared<EchoTerminalFactory>();
  tcpListener.attachToTeminalFactory(pTcpEchoTerminalFactory);

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

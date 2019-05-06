#include "Proceeders.h"

#include <chrono>
#include <thread>
#include <iostream>

namespace conveyor {

void runRealTimeProceeder(Conveyor* pConveyor)
{
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
    pConveyor->proceed(static_cast<size_t>(dt.count()));
    inGameTime += dt;
  }
}

} // namespace conveyor

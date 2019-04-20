#include <thread>
#include "Conveyor/Conveyor.h"

int main(int, char*[])
{
  uint16_t nTotalThreadsCount = 4;
  conveyor::Conveyor conveyor(nTotalThreadsCount);

  for(size_t i = 1; i < nTotalThreadsCount; ++i)
    new std::thread([&conveyor]() { conveyor.joinAsSlave();} );
  return 0;
}

#ifndef AUTOTESTS_MODE

#include <thread>
#include "ConfigDI/Containers.h"
#include "SystemManager.h"


int main(int, char*[])
{
  SystemManager app(
        config::ApplicationCfg()
        .setLoginUdpPort(6842)
        .setTotalThreads(4)
        .setPortsPool(
          config::PortsPoolCfg()
          .setBegin(25000)
          .setEnd(25100))
        );

  if (!app.initialize()) {
    std::cerr << "FAILED to initialize application!" << std::endl;
    return 1;
  }

  if (!app.start()) {
    std::cerr << "FAILED to start application!" << std::endl;
  }

  app.proceed();
}

#else

#include <gtest/gtest.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#endif

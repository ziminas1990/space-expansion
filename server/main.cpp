#ifndef AUTOTESTS_MODE

#include <thread>
#include "ConfigDI/Containers.h"
#include "ConfigDI/Readers.h"
#include "SystemManager.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

int main(int argc, char* argv[])
{
  std::string sConfigurationFile = "space-expansion.cfg";
  if (argc > 1) {
    sConfigurationFile = argv[1];
  }

  std::ifstream cfgStream;
  cfgStream.open(sConfigurationFile);
  if (!cfgStream.is_open()) {
    std::cerr << "FAILED to open configuration file \"" << sConfigurationFile << "\"!"
              << std::endl;
    return 1;
  }

  YAML::Node configuration = YAML::Load(cfgStream);

  config::ApplicationCfg applicationCfg =
      config::ApplicationCfgReader::read(configuration["application"]);

  if (!applicationCfg.isValid()) {
    std::cerr << "FAILED to read application configuration!" << std::endl;
    return 1;
  }

  SystemManager app;
  if (!app.initialize(applicationCfg)) {
    std::cerr << "FAILED to initialize application!" << std::endl;
    return 1;
  }

  if (!app.loadWorldState(configuration)) {
    std::cerr << "FAILED to load world state" << std::endl;
    return 1;
  }

  app.run(applicationCfg.isClockFreezed());
  return 0;
}

#else

#include <gtest/gtest.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#endif

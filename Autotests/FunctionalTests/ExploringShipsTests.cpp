#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class ExploringShipsFunctionalTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::stringstream ss;
    ss << "Blueprints:"           << std::endl <<
          "  CommandCenter:"      << std::endl <<
          "    weight : 4000000"  << std::endl <<
          "  Corvet:"             << std::endl <<
          "    weight : 500000"   << std::endl <<
          "  Miner:"              << std::endl <<
          "    weight : 300000"   << std::endl <<
          "  Zond:"               << std::endl <<
          "    weight : 10000";
    state = YAML::Load(ss.str());
    return true;
  }
};

TEST_F(ExploringShipsFunctionalTests, GetShipsCount)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("admin", "admin")
        .expectSuccess());

  ASSERT_TRUE(m_pRootClientCommutator->getTotalSlots(7));
}


TEST_F(ExploringShipsFunctionalTests, GetShipsTypes)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("admin", "admin")
        .expectSuccess());

  // with cycle it's even more harder
  for(size_t i = 0; i < 200; ++i) {
    ASSERT_TRUE(
          Scenarios::CheckAttachedModules(m_pRootClientCommutator)
          .hasModule("Ship/CommandCenter", 1)
          .hasModule("Ship/Miner", 2)
          .hasModule("Ship/Zond", 2)
          .hasModule("Ship/Corvet", 2)) << "on oteration #" << i;
  }
}



} // namespace autotests

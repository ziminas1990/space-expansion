#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class ExploringShipsFunctionalTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Ships:",
      "    CommandCenter:",
      "      weight : 4000000",
      "      radius : 800",
      "    Corvet:",
      "      weight : 500000",
      "      radius : 180",
      "    Miner:",
      "      weight : 300000",
      "      radius : 250",
      "    Zond:",
      "      weight : 10000",
      "      radius : 5",
      "Players:",
      "  admin:",
      "    password: admin",
      "    ships:",
      "      CommandCenter/Head:",
      "        position: { x: 0, y: 0}",
      "        velocity: { x: 0, y: 0}",
      "      Corvet/Raven:",
      "        position: { x: 15, y: 15}",
      "        velocity: { x: 0,  y: 0}",
      "      Corvet/Caracal:",
      "        position: { x: 100, y: 100}",
      "        velocity: { x: 10,  y: 10}",
      "      'Miner/Bogatstvo Narodov':",
      "        position: { x: -50, y: -90}",
      "        velocity: { x: 5,   y: -5}",
      "      'Zond/Sokol':",
      "        position: { x: 32, y: -78}",
      "        velocity: { x: -1, y: 4}"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
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

  client::ModulesList shipsInfo;
  ASSERT_TRUE(client::GetAllModules(*m_pRootCommutator, "Ship", shipsInfo));

  EXPECT_EQ(5, shipsInfo.size());
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
          Scenarios::CheckAttachedModules(m_pRootCommutator)
          .hasModule("Ship/CommandCenter", "Head")
          .hasModule("Ship/Miner",  "Bogatstvo Narodov")
          .hasModule("Ship/Zond",   "Sokol")
          .hasModule("Ship/Corvet", "Raven")
          .hasModule("Ship/Corvet", "Caracal")) << "on oteration #" << i;
  }
}

} // namespace autotests

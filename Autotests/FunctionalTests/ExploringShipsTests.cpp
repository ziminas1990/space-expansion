#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>

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
      "  CommandCenter:",
      "    weight : 4000000",
      "    radius : 800",
      "  Corvet:",
      "    weight : 500000",
      "    radius : 180",
      "  Miner:",
      "    weight : 300000",
      "    radius : 250",
      "  Zond:",
      "    weight : 10000",
      "    radius : 5",
      "Players:",
      "  admin:",
      "    password: admin",
      "    ships:",
      "      CommandCenter:",
      "        position: { x: 0, y: 0}",
      "        velocity: { x: 0, y: 0}",
      "      Corvet:",
      "        position: { x: 15, y: 15}",
      "        velocity: { x: 0,  y: 0}",
      "      Corvet:",
      "        position: { x: 100, y: 100}",
      "        velocity: { x: 10,  y: 10}",
      "      Miner:",
      "        position: { x: -50, y: -90}",
      "        velocity: { x: 5,   y: -5}",
      "      Zond:",
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

  uint32_t nTotalSlots;
  ASSERT_TRUE(m_pRootCommutator->getTotalSlots(nTotalSlots));
  EXPECT_EQ(5, nTotalSlots);
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
          .hasModule("Ship/CommandCenter", 1)
          .hasModule("Ship/Miner", 1)
          .hasModule("Ship/Zond", 1)
          .hasModule("Ship/Corvet", 2)) << "on oteration #" << i;
  }
}

TEST_F(ExploringShipsFunctionalTests, GetShipsPosition)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("admin", "admin")
        .expectSuccess());

  // Ships positions and velocities:
  std::vector<std::pair<geometry::Point, geometry::Vector>> expectedPositions =
  {
    std::make_pair(geometry::Point(0,   0),   geometry::Vector({0,  0})),
    std::make_pair(geometry::Point(15,  15),  geometry::Vector({0,  0})),
    std::make_pair(geometry::Point(100, 100), geometry::Vector({10, 10})),
    std::make_pair(geometry::Point(-50, -90), geometry::Vector({5,  -5})),
    std::make_pair(geometry::Point(32,  -78), geometry::Vector({-1, 4}))
  };

  client::Ship ship;
  geometry::Point  position;
  geometry::Vector velocity;

  for (size_t i = 0; i < 5; ++i) {
    client::TunnelPtr pTunnel = m_pRootCommutator->openTunnel(i);
    ASSERT_TRUE(pTunnel);
    ship.attachToChannel(pTunnel);
    ASSERT_TRUE(ship.getPosition(position, velocity));
    ASSERT_EQ(expectedPositions[i].first,  position);
    ASSERT_EQ(expectedPositions[i].second, velocity);
  }
}

} // namespace autotests

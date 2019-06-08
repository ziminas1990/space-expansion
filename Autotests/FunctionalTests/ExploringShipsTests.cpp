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
    std::stringstream ss;
    ss    <<              "Blueprints:"
          << std::endl << "  CommandCenter:"
          << std::endl << "    weight : 4000000"
          << std::endl << "    radius : 800"
          << std::endl << "  Corvet:"
          << std::endl << "    weight : 500000"
          << std::endl << "    radius : 180"
          << std::endl << "  Miner:"
          << std::endl << "    weight : 300000"
          << std::endl << "    radius : 250"
          << std::endl << "  Zond:"
          << std::endl << "    weight : 10000"
          << std::endl << "    radius : 5"
          << std::endl << "Players:"
          << std::endl << "  admin:"
          << std::endl << "    password: admin"
          << std::endl << "    ships:"
          << std::endl << "      CommandCenter:"
          << std::endl << "        position: { x: 0, y: 0}"
          << std::endl << "        velocity: { x: 0, y: 0}"
          << std::endl << "      Corvet:"
          << std::endl << "        position: { x: 15, y: 15}"
          << std::endl << "        velocity: { x: 0,  y: 0}"
          << std::endl << "      Corvet:"
          << std::endl << "        position: { x: 100, y: 100}"
          << std::endl << "        velocity: { x: 10,  y: 10}"
          << std::endl << "      Miner:"
          << std::endl << "        position: { x: -50, y: -90}"
          << std::endl << "        velocity: { x: 5,   y: -5}"
          << std::endl << "      Zond:"
          << std::endl << "        position: { x: 32, y: -78}"
          << std::endl << "        velocity: { x: -1, y: 4}";
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

  client::ClientShip ship;
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

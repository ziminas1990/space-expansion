#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientEngine.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class EngineControllTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Modules:",
      "    Engine:",
      "      ridiculous-engine:",
      "        max_thrust: 100",
      "      tiny-engine:",
      "        max_thrust: 200",
      "  Ships:",
      "    Cubesat:",
      "      radius: 0.1",
      "      weight: 10",
      "      modules:",
      "        engine_1: Engine/tiny-engine",
      "        engine_2: Engine/ridiculous-engine",
      "Players:",
      "  test:",
      "    password: test",
      "    ships:",
      "      'Cubesat/Small Cube':",
      "        position: { x: 100, y: 15}",
      "        velocity: { x: 10,  y: 5}",
      "        modules:",
      "          engine_1: { x: 0, y: 0}",
      "          engine_2: { x: 0, y: 0}"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }
};


TEST_F(EngineControllTests, OpenTunnelToEngine)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Small Cube", ship));

  for (size_t nSlot = 0; nSlot < 2; ++nSlot) {
    client::TunnelPtr pTunnelToEngine = ship.openTunnel(0);
    ASSERT_TRUE(pTunnelToEngine);
  }
}

TEST_F(EngineControllTests, GetSpecification)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Small Cube", ship));

  client::Engine engine;
  client::EngineSpecification specification;
  // Checking engine_1
  {
    engine.attachToChannel(ship.openTunnel(0));
    ASSERT_TRUE(engine.getSpecification(specification));
    EXPECT_EQ(200, specification.nMaxThrust);
  }

  // Checking engine_2
  {
    engine.attachToChannel(ship.openTunnel(1));
    ASSERT_TRUE(engine.getSpecification(specification));
    EXPECT_EQ(100, specification.nMaxThrust);
  }
}

TEST_F(EngineControllTests, SetAndGetThrust)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Small Cube", ship));

  client::Engine engine;
  engine.attachToChannel(ship.openTunnel(0));

  // Setting new thrust
  geometry::Vector thrust(1, 0.5);
  thrust.setLength(80);
  ASSERT_TRUE(engine.setThrust(thrust, 1000));

  // Waiting for some time
  proceedEnviroment(200);

  // Checking current thrust
  geometry::Vector currentThrust;
  ASSERT_TRUE(engine.getThrust(currentThrust));
  EXPECT_EQ(thrust, currentThrust);
}

TEST_F(EngineControllTests, MovingWithEngineTest)
{
  // This test check, that thrust on engine moves ship

  // 1. Prepharation (login & connecting to engine module)
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Small Cube", ship));

  client::Engine engine;
  ASSERT_TRUE(client::FindMostPowerfulEngine(ship, engine));

  // 2. getting current ship's position
  geometry::Point  startPosition(100, 15);
  geometry::Vector startVelocity(10, 5);
  ASSERT_TRUE(ship.getPosition(startPosition, startVelocity));

  // 3. Setting max thrust (100 H) for 5 sec
  uint32_t nIntervalSec = 5;
  geometry::Vector thrust(20, -5);
  thrust.setLength(100);
  ASSERT_TRUE(engine.setThrust(thrust, nIntervalSec * 1000));

  // 4. Waiting for 5 sec  
  proceedEnviroment(nIntervalSec * 1000, 100);

  // 5. getting current ship's position
  geometry::Point  currentPosition;
  geometry::Vector currentVelocity;
  ASSERT_TRUE(ship.getPosition(currentPosition, currentVelocity));

  // 6. calculating expected position and velocity
  double nShipMassKg = 10;
  geometry::Vector expectedAcc = thrust/nShipMassKg;
  geometry::Point expectedPosition =
      startPosition + startVelocity * nIntervalSec +
      expectedAcc * nIntervalSec * nIntervalSec * 0.5;
  geometry::Vector expectedVelocity =
      startVelocity + expectedAcc * nIntervalSec;
  EXPECT_TRUE(currentPosition.almostEqual(expectedPosition, 0.1));
  EXPECT_TRUE(currentVelocity.almostEqual(expectedVelocity, 0.1));
}

} // namespace autotests

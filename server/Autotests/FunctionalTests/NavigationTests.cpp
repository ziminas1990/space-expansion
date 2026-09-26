#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientRCS.h>
#include <Autotests/ClientSDK/Procedures/Navigation.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

#include <yaml-cpp/yaml.h>
#include <sstream>
#include <iostream>

namespace autotests
{

class NavigationTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Modules:",
      "    RCS:",
      "      tiny-engine:",
      "        max_thrust: 200",
      "        expenses:",
      "          labor: 1",
      "  Ships:",
      "    Cubesat:",
      "      radius:  0.1",
      "      weight:  10 ",
      "      max_rotation_speed: 1",
      "      modules:",
      "        rcs: RCS/tiny-engine",
      "      expenses:",
      "        labor: 10",
      "Players:",
      "  test:",
      "    password: test",
      "    ships:",
      "      Cubesat/Experimental:",
      "        position: { x: 0, y: 0}",
      "        velocity: { x: 0, y: 0}",
      "        orientation: { x: 1, y: 0}",
      "        modules:",
      "          rcs: { x: 0, y: 0}"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }

  bool checkPosition(client::ShipPtr pShip, geometry::Point const& target)
  {
    geometry::Point  position;
    geometry::Vector velocity;
    if (!pShip->getPosition(position, velocity))
      return false;
    return position.distance(target) <= 1 && velocity.getLength() <= 1;
  }
};


TEST_F(NavigationTests, SimpleTest)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::ShipPtr pShip = std::make_shared<client::Ship>(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Experimental", *pShip));

  client::Navigation navigation(pShip);
  ASSERT_TRUE(navigation.initialize());

  geometry::Point target(100, 100);
  ASSERT_TRUE(Scenarios::RunProcedures()
              .add(navigation.MakeMoveToProcedure(target))
              .wait(20, 500));

  ASSERT_TRUE(checkPosition(pShip, target));
}

TEST_F(NavigationTests, SeveralPoints)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::ShipPtr pShip = std::make_shared<client::Ship>(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Experimental", *pShip));

  client::Navigation navigation(pShip);
  ASSERT_TRUE(navigation.initialize());

  std::vector<geometry::Point> path = {
    geometry::Point( 100,  100),
    geometry::Point( 50,  -200),
    geometry::Point(-100,  50),
    geometry::Point(-50,  -50),
    geometry::Point( 0,    0)
  };
  for (geometry::Point const& target : path)
  {
    ASSERT_TRUE(Scenarios::RunProcedures()
                .add(navigation.MakeMoveToProcedure(target))
                .wait(20, 500));
    ASSERT_TRUE(checkPosition(pShip, target));
  }
}

TEST_F(NavigationTests, OnMoving)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::ShipPtr pShip = std::make_shared<client::Ship>(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Experimental", *pShip));

  client::RCS engine;
  engine.attachToChannel(pShip->openSession(0));

  // Setting new thrust and waiting for 3 seconds
  geometry::Vector thrust(-1, -0.5);
  thrust.setLength(100);
  ASSERT_TRUE(engine.setThrust(thrust, 3000));
  skipTime(3000);

  // Now we have non-zero started velocity
  // Moving to some point
  client::Navigation navigation(pShip);
  ASSERT_TRUE(navigation.initialize());

  geometry::Point target(-47, 160);
  ASSERT_TRUE(Scenarios::RunProcedures()
              .add(navigation.MakeMoveToProcedure(target))
              .wait(20, 500));
  ASSERT_TRUE(checkPosition(pShip, target));
}

TEST_F(NavigationTests, ThrustStaysConstantWhileShipRotates)
{
  // 1. player logins and opens the ship
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::ShipPtr pShip = std::make_shared<client::Ship>(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Experimental", *pShip));

  client::RCS engine;
  engine.attachToChannel(pShip->openSession(0));

  // 2. turn the thrusters on along (1, 0)
  //    the applied force is the module maximum in that direction
  client::RCSSpecification spec;
  ASSERT_TRUE(engine.getSpecification(spec));
  const geometry::Vector direction(1, 0);
  const geometry::Vector thrust = direction.ofLength(spec.nMaxThrust);
  const uint32_t nBurnMs = 20000;
  ASSERT_TRUE(engine.setThrust(direction, nBurnMs));

  // 3. start a half turn, so the nose goes from +X to -X
  ASSERT_TRUE(pShip->rotate(geometry::Vector(-1, 0), 1));

  // 4. wait until the turn is finished and the ship has flown a bit further
  geometry::Point  startPosition;
  geometry::Vector startVelocity;
  ASSERT_TRUE(pShip->getPosition(startPosition, startVelocity));
  const uint64_t t0 = m_application.getClock().now();
  const uint32_t nWaitMs = 5000;
  skipTime(nWaitMs);
  const double t = (m_application.getClock().now() - t0) / 1000000.0;

  // 5. thrust is still the vector that was set
  geometry::Vector currentThrust;
  ASSERT_TRUE(engine.getThrust(currentThrust));
  EXPECT_EQ(thrust, currentThrust);

  // 6. the nose has turned to the opposite direction
  client::ShipState ship;
  ASSERT_TRUE(pShip->getState(ship));
  EXPECT_TRUE(ship.orientation.almostEqual(geometry::Vector(-1, 0), 1e-4));

  // 7. position matches constant acceleration along the original thrust
  geometry::Point  position;
  geometry::Vector velocity;
  ASSERT_TRUE(pShip->getPosition(position, velocity));
  const geometry::Vector acceleration = thrust / ship.nWeight;
  const geometry::Vector expectedVelocity = startVelocity + acceleration * t;
  const geometry::Point  expectedPosition =
      startPosition + startVelocity * t + acceleration * (t * t * 0.5);
  EXPECT_TRUE(velocity.almostEqual(expectedVelocity, 1e-4));
  EXPECT_TRUE(position.almostEqual(expectedPosition, 1e-3))
      << position << " != " << expectedPosition;
}

} // namespace autotests

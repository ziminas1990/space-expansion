#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientEngine.h>
#include <Autotests/ClientSDK/Procedures/Navigation.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class NavigationTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Cubesat:",
      "    radius:   0.1",
      "    weight: 10 ",
      "    modules: ",
      "      engine: ",
      "        type:      engine",
      "        maxThrust: 200",
      "Players:",
      "  test:",
      "    password: test",
      "    ships:",
      "      Cubesat/Experimental:",
      "        position: { x: 0, y: 0}",
      "        velocity: { x: 0, y: 0}",
      "        modules:",
      "          engine: { x: 0, y: 0}"
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

  client::ShipPtr pShip = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Experimental", *pShip));

  client::Navigation navigation(pShip);
  ASSERT_TRUE(navigation.initialize());

  geometry::Point target(100, 100);
  ASSERT_TRUE(Scenarios::RunProcedures()
              .add(navigation.MakeMoveToProcedure(target, 100))
              .wait(20, 500));

  ASSERT_TRUE(checkPosition(pShip, target));
}

TEST_F(NavigationTests, SeveralPoints)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());

  client::ShipPtr pShip = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Experimental", *pShip));

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
                .add(navigation.MakeMoveToProcedure(target, 100))
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

  client::ShipPtr pShip = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Experimental", *pShip));

  client::Engine engine;
  engine.attachToChannel(pShip->openTunnel(0));

  // Setting new thrust and waiting for 3 seconds
  geometry::Vector thrust(-1, -0.5);
  thrust.setLength(100);
  ASSERT_TRUE(engine.setThrust(thrust, 3000));
  proceedEnviroment(3000);

  // Now we have non-zero started velocity
  // Moving to some point
  client::Navigation navigation(pShip);
  ASSERT_TRUE(navigation.initialize());

  geometry::Point target(-47, 160);
  ASSERT_TRUE(Scenarios::RunProcedures()
              .add(navigation.MakeMoveToProcedure(target, 100))
              .wait(20, 500));
  ASSERT_TRUE(checkPosition(pShip, target));
}

} // namespace autotests

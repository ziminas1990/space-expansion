#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientEngine.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class ShipTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Modules:",
      "    Engine:",
      "      tiny-engine:",
      "        max_thrust: 200",
      "        expenses:",
      "          labor: 1",
      "  Ships:",
      "    Cubesat:",
      "      radius:  0.1",
      "      weight:  10 ",
      "      modules:",
      "        engine: Engine/tiny-engine",
      "      expenses:",
      "        labor: 10",
      "Players:",
      "  test:",
      "    password: test",
      "    ships:",
      "      Cubesat/Experimental:",
      "        position: { x: 100, y: 100}",
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

};

TEST_F(ShipTests, Monitoring)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());

  client::ShipPtr pShip = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Experimental", *pShip));

  client::Engine engine;
  engine.attachToChannel(pShip->openTunnel(0));

  uint32_t nMontorAck = 0;
  client::ShipState state;

  ASSERT_TRUE(pShip->monitor(500, nMontorAck));
  ASSERT_EQ(500, nMontorAck);
  ASSERT_TRUE(pShip->waitState(state));

  resumeTime();
  for (size_t i = 1; i < 100; ++i) {
    uint64_t expectedTime = m_application.getClock().now() + 500000;
    ASSERT_TRUE(pShip->waitState(state)) << "On iteration " << i;
    uint64_t now = m_application.getClock().now();
    ASSERT_NEAR(expectedTime, now, 10000) << "On iteration " << i;
  }

  ASSERT_TRUE(pShip->monitor(1000, nMontorAck));
  ASSERT_EQ(1000, nMontorAck);
  ASSERT_TRUE(pShip->waitState(state));

  for (size_t i = 0; i < 10; ++i) {
    uint64_t expectedTime = m_application.getClock().now() + 1000000;
    ASSERT_TRUE(pShip->waitState(state)) << "On iteration " << i;
    uint64_t now = m_application.getClock().now();
    ASSERT_NEAR(expectedTime, now, 10000) << "On iteration " << i;
  }

  ASSERT_TRUE(pShip->monitor(0, nMontorAck));
  ASSERT_EQ(0, nMontorAck);
  ASSERT_FALSE(pShip->waitState(state, 50));
}

TEST_F(ShipTests, MonitoringLimits)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());

  client::ShipPtr pShip = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Experimental", *pShip));

  client::Engine engine;
  engine.attachToChannel(pShip->openTunnel(0));

  uint32_t nMontorAck = 0;
  client::ShipState state;

  // Period is too big
  ASSERT_TRUE(pShip->monitor(100000, nMontorAck));
  ASSERT_EQ(60000, nMontorAck);
  ASSERT_TRUE(pShip->waitState(state));

  // Period is two short
  ASSERT_TRUE(pShip->monitor(50, nMontorAck));
  ASSERT_EQ(100 , nMontorAck);
  ASSERT_TRUE(pShip->waitState(state));
}

} // namespace autotests

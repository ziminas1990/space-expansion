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

  uint32_t nMonitorAck = 0;
  client::ShipState state;

  ASSERT_TRUE(pShip->monitor(500, nMonitorAck));
  ASSERT_EQ(500, nMonitorAck);
  ASSERT_TRUE(pShip->waitState(state));

  resumeTime();
  for (size_t i = 1; i < 100; ++i) {
    uint64_t expectedTime = m_application.getClock().now() + 500000;
    ASSERT_TRUE(pShip->waitState(state)) << "On iteration " << i;
    uint64_t now = m_application.getClock().now();
    ASSERT_NEAR(expectedTime, now, 10000) << "On iteration " << i;
  }

  ASSERT_TRUE(pShip->monitor(1000, nMonitorAck));
  ASSERT_EQ(1000, nMonitorAck);
  ASSERT_TRUE(pShip->waitState(state));

  for (size_t i = 0; i < 10; ++i) {
    uint64_t expectedTime = m_application.getClock().now() + 1000000;
    ASSERT_TRUE(pShip->waitState(state)) << "On iteration " << i;
    uint64_t now = m_application.getClock().now();
    ASSERT_NEAR(expectedTime, now, 10000) << "On iteration " << i;
  }

  ASSERT_TRUE(pShip->monitor(0, nMonitorAck));
  ASSERT_EQ(0, nMonitorAck);
  ASSERT_FALSE(pShip->waitState(state, 200));
}

TEST_F(ShipTests, MultipleSubscriptions)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("test", "test")
        .expectSuccess());

  client::ShipPtr pShip = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Experimental", *pShip));

  using Subscription = std::pair<client::ShipPtr, uint32_t>;

  Subscription subscriptions[] = {
    {std::make_shared<client::Ship>(), 200},
    {std::make_shared<client::Ship>(), 300},
    {std::make_shared<client::Ship>(), 400}
  };
  size_t total = sizeof(subscriptions) / sizeof(Subscription);

  for (Subscription& subscription: subscriptions) {
    ASSERT_TRUE(client::attachToShip(
                  m_pRootCommutator, "Experimental", *subscription.first));
    uint32_t nMonitorAck = 0;
    ASSERT_TRUE(subscription.first->monitor(subscription.second, nMonitorAck));
    ASSERT_EQ(subscription.second, nMonitorAck);
  }

  client::ShipState state;

  for (Subscription& subscription: subscriptions) {
    ASSERT_TRUE(subscription.first->waitState(state));
  }

  resumeTime();
  for (size_t i = 1; i < 10; ++i) {
    for (Subscription& subscription: subscriptions) {
      ASSERT_TRUE(subscription.first->waitState(state));
    }
  }

  // Unsubscribing
  for (size_t i = 0; i < total; ++i) {
    {
      Subscription& subscription = subscriptions[i];
      subscription.first->dropQueuedMessage();
      uint32_t nMonitorAck = 0;
      ASSERT_TRUE(subscription.first->monitor(0, nMonitorAck));
      ASSERT_FALSE(subscription.first->waitState(state, 100)) << i;
    }
    for (size_t j = 0; j < total; ++j) {
      Subscription& subscription = subscriptions[j];
      if (j <= i) {
        ASSERT_FALSE(subscription.first->waitState(state, 100)) << j << "/" << i;
      } else {
        ASSERT_TRUE(subscription.first->waitState(state, 100)) << j << "/" << i;
      }
    }
  }
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

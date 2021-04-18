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

  client::ShipPtr pShipMonitor = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Experimental", *pShipMonitor));
  uint32_t subscription;
  ASSERT_EQ(client::ClientBaseModule::eSuccess, pShipMonitor->subscribe(subscription));

  uint32_t nMonitorAck = 0;
  client::ShipState state;

  ASSERT_TRUE(pShip->monitor(500, nMonitorAck));
  ASSERT_EQ(500, nMonitorAck);
  ASSERT_TRUE(pShipMonitor->waitState(state));

  resumeTime();
  for (size_t i = 1; i < 100; ++i) {
    uint64_t expectedTime = m_application.getClock().now() + 500000;
    ASSERT_TRUE(pShipMonitor->waitState(state)) << "On iteration " << i;
    uint64_t now = m_application.getClock().now();
    ASSERT_NEAR(expectedTime, now, 10000) << "On iteration " << i;
  }

  ASSERT_TRUE(pShip->monitor(1000, nMonitorAck));
  ASSERT_EQ(1000, nMonitorAck);
  ASSERT_TRUE(pShipMonitor->waitState(state));

  for (size_t i = 0; i < 10; ++i) {
    uint64_t expectedTime = m_application.getClock().now() + 1000000;
    ASSERT_TRUE(pShipMonitor->waitState(state)) << "On iteration " << i;
    uint64_t now = m_application.getClock().now();
    ASSERT_NEAR(expectedTime, now, 10000) << "On iteration " << i;
  }

  ASSERT_TRUE(pShip->monitor(0, nMonitorAck));
  ASSERT_EQ(0, nMonitorAck);
  ASSERT_FALSE(pShipMonitor->waitState(state, 200));
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
    {std::make_shared<client::Ship>(), 0},
    {std::make_shared<client::Ship>(), 0},
    {std::make_shared<client::Ship>(), 0}
  };
  size_t total = sizeof(subscriptions) / sizeof(Subscription);

  for (Subscription& subscription: subscriptions) {
    ASSERT_TRUE(client::attachToShip(
                  m_pRootCommutator, "Experimental", *subscription.first));
    ASSERT_EQ(client::ClientBaseModule::eSuccess,
              subscription.first->subscribe(subscription.second));
  }

  uint32_t nMonitorAck = 0;
  client::ShipState state;

  ASSERT_TRUE(pShip->monitor(500, nMonitorAck));
  ASSERT_EQ(500, nMonitorAck);
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
    ASSERT_EQ(client::ClientBaseModule::eInvalidToken,
              pShip->unsubscribe(subscriptions[i].second + 1));
    ASSERT_EQ(client::ClientBaseModule::eSuccess,
              pShip->unsubscribe(subscriptions[i].second));
    for (size_t j = 0; j < total; ++j) {
      Subscription& subscription = subscriptions[j];
      if (j <= i) {
        ASSERT_FALSE(subscription.first->waitState(state, 100));
      } else {
        ASSERT_TRUE(subscription.first->waitState(state, 100));
        // Removing all other updates that may have been received
        subscription.first->dropQueuedMessage();
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
  client::ShipPtr pShipMonitor = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Experimental", *pShip));
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Experimental", *pShipMonitor));

  uint32_t subscription;
  ASSERT_EQ(client::ClientBaseModule::eSuccess, pShipMonitor->subscribe(subscription));

  client::Engine engine;
  engine.attachToChannel(pShip->openTunnel(0));

  uint32_t nMontorAck = 0;
  client::ShipState state;

  // Period is too big
  ASSERT_TRUE(pShip->monitor(100000, nMontorAck));
  ASSERT_EQ(60000, nMontorAck);
  ASSERT_TRUE(pShipMonitor->waitState(state));

  // Period is two short
  ASSERT_TRUE(pShip->monitor(50, nMontorAck));
  ASSERT_EQ(100 , nMontorAck);
  ASSERT_TRUE(pShipMonitor->waitState(state));
}

} // namespace autotests

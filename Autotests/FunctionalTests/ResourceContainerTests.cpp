#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientResourceContainer.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class ResourceContainerTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Freighter:",
      "    radius: 100",
      "    weight: 100000 ",
      "    modules:",
      "      cargo:",
      "        type:   ResourceContainer",
      "        volume: 1000",
      "      engine: ",
      "        type:      engine",
      "        maxThrust: 5000000",
      "Players:",
      "  merchant:",
      "    password: money",
      "    ships:",
      "      Freighter:",
      "        position: { x: 0, y: 0}",
      "        velocity: { x: 0, y: 0}",
      "        modules:",
      "          engine: { x: 0, y: 0}",
      "          cargo:  {",
      "            mettals:   100000,",
      "            silicates: 50000,",
      "            ice:       250000",
      "          }"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }
};

TEST_F(ResourceContainerTests, GetContent)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("merchant", "money")
        .expectSuccess());

  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
  ASSERT_TRUE(pTunnelToShip);

  client::Ship ship;
  ship.attachToChannel(pTunnelToShip);

  client::ResourceContainer container;
  ASSERT_TRUE(client::FindResourceContainer(ship, container));

  client::ResourceContainer::Content content;
  ASSERT_TRUE(container.getContent(content));
  EXPECT_EQ(1000, content.m_nVolume);
  EXPECT_NEAR(100000, content.m_amount[world::Resource::eMettal],   0.1);
  EXPECT_NEAR(50000,  content.m_amount[world::Resource::eSilicate], 0.1);
  EXPECT_NEAR(250000, content.m_amount[world::Resource::eIce],      0.1);
  EXPECT_NEAR(316.6,  content.m_nUsedSpace, 0.1);
}

TEST_F(ResourceContainerTests, OpenPort)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("merchant", "money")
        .expectSuccess());

  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
  ASSERT_TRUE(pTunnelToShip);

  client::Ship ship;
  ship.attachToChannel(pTunnelToShip);

  client::ResourceContainer container;
  ASSERT_TRUE(client::FindResourceContainer(ship, container));

  uint32_t nAccessKey = 100500;
  uint32_t nPortId    = 0;
  ASSERT_EQ(container.openPort(nAccessKey, nPortId),
            client::ResourceContainer::eStatusOk);
  ASSERT_NE(0, nPortId);

  ASSERT_EQ(container.openPort(42, nPortId),
            client::ResourceContainer::eStatusPortAlreadyOpen);
}

TEST_F(ResourceContainerTests, ClosePort)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("merchant", "money")
        .expectSuccess());

  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
  ASSERT_TRUE(pTunnelToShip);

  client::Ship ship;
  ship.attachToChannel(pTunnelToShip);

  client::ResourceContainer container;
  ASSERT_TRUE(client::FindResourceContainer(ship, container));

  ASSERT_EQ(container.closePort(), client::ResourceContainer::eStatusPortIsNotOpened);

  for (uint32_t i = 0; i < 5; ++i) {
    uint32_t nAccessKey = i;
    uint32_t nPortId    = 0;
    ASSERT_EQ(container.openPort(nAccessKey, nPortId),
              client::ResourceContainer::eStatusOk);
    ASSERT_NE(0, nPortId);

    ASSERT_EQ(container.closePort(), client::ResourceContainer::eStatusOk);
  }
}

} // namespace autotests

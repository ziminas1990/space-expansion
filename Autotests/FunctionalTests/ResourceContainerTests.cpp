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
      "  Modules:",
      "    ResourceContainer:",
      "      small-cargo:",
      "        volume: 1000",
      "        expenses:",
      "          labor: 10",
      "      huge-cargo:",
      "        volume: 100000",
      "        expenses:",
      "          labor: 100",
      "    Engine:",
      "      huge-engine:",
      "        max_thrust: 5000000",
      "        expenses:",
      "          labor: 100",
      "  Ships:",
      "    Freighter:",
      "      radius: 100",
      "      weight: 100000 ",
      "      modules:",
      "        cargo:  ResourceContainer/small-cargo",
      "        engine: Engine/huge-engine",
      "      expenses:",
      "        labor: 1000",
      "    Station:",
      "      radius: 1000",
      "      weight: 10000000",
      "      modules:",
      "        cargo: ResourceContainer/huge-cargo",
      "      expenses:",
      "        labor: 1000",
      "Players:",
      "  merchant:",
      "    password: money",
      "    ships:",
      "      'Freighter/Freighter One' :",
      "        position: { x: 1100, y: 200}",
      "        velocity: { x: 0,  y: 0 }",
      "        modules:",
      "          engine: { x: 0, y: 0}",
      "          cargo:",
      "            metals:    100000",
      "            silicates: 50000",
      "            ice:       250000",
      "      'Station/Earth Hub' :",
      "        position: { x: 0, y: 0}",
      "        velocity: { x: 0, y: 0}",
      "        modules:",
      "          cargo:",
      "            metals:    1000000",
      "            silicates: 1000000",
      "            ice:       1000000",
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
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("merchant", "money")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Freighter One", ship));

  client::ResourceContainer container;
  ASSERT_TRUE(client::FindResourceContainer(ship, container));

  client::ResourceContainer::Content content;
  ASSERT_TRUE(container.getContent(content));
  EXPECT_EQ(1000, content.m_nVolume);
  EXPECT_NEAR(100000, content.m_amount[world::Resource::eMetal],   0.1);
  EXPECT_NEAR(50000,  content.m_amount[world::Resource::eSilicate], 0.1);
  EXPECT_NEAR(250000, content.m_amount[world::Resource::eIce],      0.1);
  EXPECT_NEAR(316.6,  content.m_nUsedSpace, 0.1);
}

TEST_F(ResourceContainerTests, OpenPort)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("merchant", "money")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Freighter One", ship));

  client::ResourceContainer container;
  ASSERT_TRUE(client::FindResourceContainer(ship, container));

  uint32_t nAccessKey = 100500;
  uint32_t nPortId    = 0;
  ASSERT_EQ(container.openPort(nAccessKey, nPortId),
            client::ResourceContainer::eStatusOk);
  ASSERT_NE(0, nPortId);

  ASSERT_EQ(container.openPort(42, nPortId),
            client::ResourceContainer::ePortAlreadyOpen);
}

TEST_F(ResourceContainerTests, ClosePort)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("merchant", "money")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Freighter One", ship));

  client::ResourceContainer container;
  ASSERT_TRUE(client::FindResourceContainer(ship, container));

  ASSERT_EQ(container.closePort(), client::ResourceContainer::ePortIsNotOpened);

  for (uint32_t i = 0; i < 5; ++i) {
    uint32_t nAccessKey = i;
    uint32_t nPortId    = 0;
    ASSERT_EQ(container.openPort(nAccessKey, nPortId),
              client::ResourceContainer::eStatusOk);
    ASSERT_NE(0, nPortId);

    ASSERT_EQ(container.closePort(), client::ResourceContainer::eStatusOk);
  }
}


TEST_F(ResourceContainerTests, TransferSuccessCase)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("merchant", "money")
        .expectSuccess());

  client::Ship freighter;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Freighter One", freighter));

  client::Ship station;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Earth Hub", station));

  client::ResourceContainer          stationsContainer;
  client::ResourceContainer::Content stationContent;
  ASSERT_TRUE(client::FindResourceContainer(station, stationsContainer, "cargo"));
  ASSERT_TRUE(stationsContainer.getContent(stationContent));

  client::ResourceContainer          freighterContainer;
  client::ResourceContainer::Content freigherContent;
  ASSERT_TRUE(client::FindResourceContainer(freighter, freighterContainer, "cargo"));
  ASSERT_TRUE(freighterContainer.getContent(freigherContent));

  uint32_t nAccessKey = 43728;
  uint32_t nPort      = 0;
  ASSERT_EQ(client::ResourceContainer::eStatusOk,
            stationsContainer.openPort(nAccessKey, nPort));

  // transporting all metals
  ASSERT_EQ(client::ResourceContainer::eStatusOk,
            freighterContainer.transferRequest(
              nPort, nAccessKey, world::Resource::eMetal, freigherContent.metals()));
  ASSERT_EQ(client::ResourceContainer::eStatusOk,
            freighterContainer.waitTransfer(
              world::Resource::eMetal, freigherContent.metals()));

  stationContent.addMetals(freigherContent.metals());
  freigherContent.setMetals(0);
  ASSERT_TRUE(freighterContainer.checkContent(freigherContent));
  ASSERT_TRUE(stationsContainer.checkContent(stationContent));

  // transporting silicates (partially)
  ASSERT_EQ(client::ResourceContainer::eStatusOk,
            freighterContainer.transferRequest(
              nPort, nAccessKey, world::Resource::eSilicate,
              freigherContent.silicates() / 2));
  ASSERT_EQ(client::ResourceContainer::eStatusOk,
            freighterContainer.waitTransfer(
              world::Resource::eSilicate, freigherContent.silicates() / 2));

  stationContent.addSilicates(freigherContent.silicates() / 2);
  freigherContent.setSilicates(freigherContent.silicates() / 2);
  ASSERT_TRUE(freighterContainer.checkContent(freigherContent));
  ASSERT_TRUE(stationsContainer.checkContent(stationContent));

  // transporting all ice
  ASSERT_EQ(client::ResourceContainer::eStatusOk,
            freighterContainer.transferRequest(
              nPort, nAccessKey, world::Resource::eIce,
              freigherContent.ice()));
  ASSERT_EQ(client::ResourceContainer::eStatusOk,
            freighterContainer.waitTransfer(
              world::Resource::eIce, freigherContent.ice()));

  stationContent.addIce(freigherContent.ice());
  freigherContent.setIce(0);
  ASSERT_TRUE(freighterContainer.checkContent(freigherContent));
  ASSERT_TRUE(stationsContainer.checkContent(stationContent));
}

} // namespace autotests

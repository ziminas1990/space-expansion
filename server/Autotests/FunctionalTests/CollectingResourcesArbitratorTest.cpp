#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientResourceContainer.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>
#include <Autotests/ClientSDK/Procedures/Resources.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class CollectingResourcesArbitratorTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:"
      ,"  Modules:"
      ,"    ResourceContainer:"
      ,"      huge-cargo:"
      ,"        volume: 100000"
      ,"        expenses:"
      ,"          labor: 100"
      ,"    Shipyard:"
      ,"      shipyard:"
      ,"        productivity:   5"
      ,"        container_name: cargo"
      ,"        expenses:"
      ,"          labor: 100"
      ,"  Ships:"
      ,"    Freighter:"
      ,"      radius: 100"
      ,"      weight: 100000 "
      ,"      modules:"
      ,"        cargo:  ResourceContainer/huge-cargo"
      ,"      expenses:"
      ,"        labor: 1000"
      ,"    Station:"
      ,"      radius: 1000"
      ,"      weight: 10000000"
      ,"      modules:"
      ,"        cargo:    ResourceContainer/huge-cargo"
      ,"        shipyard: Shipyard/shipyard"
      ,"      expenses:"
      ,"        labor: 1000"
      ,"Players:"
      ,"  Buffet:"
      ,"    password: Money"
      ,"    ships:"
      ,"      'Freighter/Mule' :"
      ,"        position: { x: 1050, y: 0}"
      ,"        velocity: { x: 0,    y: 0}"
      ,"        modules:"
      ,"          cargo:"
      ,"            metals:    500"
      ,"            silicates: 500"
      ,"            ice:       500"
      ,"      'Station/Hub#1':"
      ,"        position: { x: 0, y: 0}"
      ,"        velocity: { x: 0, y: 0}"
      ,"        modules:"
      ,"          cargo:"
      ,"            metals:    300"
      ,"            silicates: 300"
      ,"            ice:       300"
      ,"      'Station/Hub#2':"
      ,"        position: { x: 2100, y: 0}"
      ,"        velocity: { x: 0,    y: 0}"
      ,"        modules:"
      ,"          cargo:"
      ,"            metals:    300"
      ,"            silicates: 300"
      ,"            ice:       300"
      ,"  Trader#1:"
      ,"    password: nomoney"
      ,"    ships:"
      ,"      'Station/Fail':"
      ,"        position: { x: 2100, y: 100}"
      ,"        velocity: { x: 0,    y: 0}"
      ,"        modules:"
      ,"          cargo:"
      ,"            metals:    10000"
      ,"  Trader#2:"
      ,"    password: nomoney"
      ,"    ships:"
      ,"      'Station/Fail':"
      ,"        position: { x: 2100, y: -100}"
      ,"        velocity: { x: 0,    y: 0}"
      ,"        modules:"
      ,"          cargo:"
      ,"            silicates: 500"
      ,"Arbitrator:"
      ,"  name: 'Collecting Resources'"
      ,"  target_score: 1000000"
      ,"  resources:"
      ,"    metals:    1000"
      ,"    silicates: 1000"
      ,"    ice:       1000"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }
};

TEST_F(CollectingResourcesArbitratorTests, BreathTest)
{
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Buffet", "Money")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::Ship freighter(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Mule", freighter));

  client::Ship hub_1(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Hub#1", hub_1));

  client::Ship hub_2(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Hub#2", hub_2));
}

TEST_F(CollectingResourcesArbitratorTests, DISABLED_SuccessCase)
{
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Buffet", "Money")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::Ship freighter(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Mule", freighter));

  client::Ship hub_1(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Hub#1", hub_1));

  client::Ship hub_2(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Hub#2", hub_2));

  spex::IGame::GameOver report;
  ASSERT_FALSE(pCommutator->waitGameOverReport(report, 50));

  // Moving mettals to hub_1
  ASSERT_TRUE(client::ResourcesManagment::transfer(
                freighter, "cargo", hub_1, "cargo",
                world::ResourcesArray().metals(500)));
  ASSERT_FALSE(pCommutator->waitGameOverReport(report, 50));

  // Moving silicates to hub_2
  ASSERT_TRUE(client::ResourcesManagment::transfer(
                freighter, "cargo", hub_2, "cargo",
                world::ResourcesArray().silicates(500)));
  ASSERT_FALSE(pCommutator->waitGameOverReport(report, 50));

  // Splitting ice between to hub_1 and hub_2
  ASSERT_TRUE(client::ResourcesManagment::transfer(
                freighter, "cargo", hub_1, "cargo",
                world::ResourcesArray().ice(250)));
  ASSERT_FALSE(pCommutator->waitGameOverReport(report, 50));
  ASSERT_TRUE(client::ResourcesManagment::transfer(
                freighter, "cargo", hub_2, "cargo",
                world::ResourcesArray().ice(250)));

  // Expecting to get game_over
  ASSERT_TRUE(pCommutator->waitGameOverReport(report));
  ASSERT_EQ(3, report.leaders().size());

  EXPECT_EQ("Buffet", report.leaders(0).player());
  EXPECT_EQ(1000000,   report.leaders(0).score());

  EXPECT_EQ("Trader#1", report.leaders(1).player());
  EXPECT_EQ(1000000 / 3, report.leaders(1).score());

  EXPECT_EQ("Trader#2", report.leaders(2).player());
  EXPECT_EQ(1000000 / 6, report.leaders(2).score());
}

} // namespace autotests

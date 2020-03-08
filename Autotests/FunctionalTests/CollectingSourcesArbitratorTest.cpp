#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientResourceContainer.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

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
      ,"      'Freighter/Freighter One' :"
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
      ,"        position: { x: 2100, y: 0}"
      ,"        velocity: { x: 0,    y: 0}"
      ,"        modules:"
      ,"          cargo:"
      ,"            metals:    200"
      ,"  Trader#2:"
      ,"    password: nomoney"
      ,"    ships:"
      ,"      'Station/Fail':"
      ,"        position: { x: 2100, y: 0}"
      ,"        velocity: { x: 0,    y: 0}"
      ,"        modules:"
      ,"          cargo:"
      ,"            silicates: 300"
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
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Buffet", "Money")
        .expectSuccess());

  client::Ship freighter;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Freighter One", freighter));

  client::Ship hub_1;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Hub#1", hub_1));

  client::Ship hub_2;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Hub#2", hub_2));
}

} // namespace autotests

#include "FunctionalTestFixture.h"

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include <sstream>

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientShipyard.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

#include "Scenarios.h"

namespace autotests
{

class ShipyardTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
       "Blueprints:"
      ,"  Modules:"
      ,"    Shipyard:"
      ,"      small-shipyard:"
      ,"        productivity:   254"
      ,"        container_name: shipyard_container"
      ,"        expenses:"
      ,"          labor: 100"
      ,"    ResourceContainer:"
      ,"      small-container:"
      ,"        volume: 100"
      ,"        expenses:"
      ,"          labor: 10"
      ,"  Ships:"
      ,"    Station:"
      ,"      radius: 100"
      ,"      weight: 100000"
      ,"      modules:"
      ,"        shipyard: Shipyard/small-shipyard"
      ,"      expenses:"
      ,"        labor: 1000"
      ,"Players:"
      ,"  Jack:"
      ,"    password: Black"
      ,"    ships:"
      ,"      'Station/Sweet Home':"
      ,"        position: { x: 100, y: 15}"
      ,"        velocity: { x: 0,   y: 0}"
      ,"        modules:"
      ,"          shipyard: {}"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }
};


TEST_F(ShipyardTests, BreathTest)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Jack", "Black")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Sweet Home", ship));

  client::Shipyard shipyard;
  ASSERT_TRUE(client::FindShipyard(ship, shipyard, "shipyard"));
}

TEST_F(ShipyardTests, GetSpecification)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Jack", "Black")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Sweet Home", ship));

  client::Shipyard shipyard;
  ASSERT_TRUE(client::FindShipyard(ship, shipyard, "shipyard"));

  client::ShipyardSpecification spec;
  ASSERT_TRUE(shipyard.getSpecification(spec));

  EXPECT_DOUBLE_EQ(254, spec.m_nLaborPerSec);
}

} // namespace autotests

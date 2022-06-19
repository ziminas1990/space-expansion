#include "FunctionalTestFixture.h"

#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>
#include <sstream>

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientShipyard.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>
#include <Autotests/ClientSDK/Procedures/Resources.h>
#include <Autotests/ClientSDK/Modules/ClientBlueprintStorage.h>
#include <Utils/Stopwatch.h>

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
      ,"        productivity:   5"
      ,"        expenses:"
      ,"          labor: 100"
      ,"    ResourceContainer:"
      ,"      huge-container:"
      ,"        volume: 100"
      ,"        expenses:"
      ,"          labor: 100"
      ,"      small-container:"
      ,"        volume: 5"
      ,"        expenses:"
      ,"          labor:     10"
      ,"          metals:    10"
      ,"          silicates: 5"
      ,"    Engine:"
      ,"      toy-engine:"
      ,"        max_thrust: 500"
      ,"        expenses:"
      ,"          labor:     10"
      ,"          metals:    5"
      ,"          silicates: 5"
      ,"          ice:       5"
      ,"    AsteroidScanner:"
      ,"      toy-scanner:"
      ,"        max_scanning_distance:  1000"
      ,"        scanning_time_ms:       100"
      ,"        expenses:"
      ,"          labor:     10"
      ,"          metals:    2"
      ,"          silicates: 5"
      ,"    AsteroidMiner:"
      ,"      toy-miner:"
      ,"        max_distance:     200"
      ,"        cycle_time_ms:    1000"
      ,"        yield_per_cycle:  100"
      ,"        container:        cargo"
      ,"        expenses:"
      ,"          labor: 100"
      ,"  Ships:"
      ,"    Station:"
      ,"      radius: 100"
      ,"      weight: 100000"
      ,"      modules:"
      ,"        shipyard:       Shipyard/small-shipyard"
      ,"        shipyard_cargo: ResourceContainer/small-container"
      ,"        cargo:          ResourceContainer/huge-container"
      ,"      expenses:"
      ,"        labor: 10000"
      ,"    MiningDrone:"
      ,"      radius: 2"
      ,"      weight: 120"
      ,"      modules:"
      ,"        engine: Engine/toy-engine"
      ,"        cargo:  ResourceContainer/small-container"
      ,"        miner:  AsteroidMiner/toy-miner"
      ,"      expenses:"
      ,"        labor:     100"
      ,"        metals:    20"
      ,"        silicates: 10"
      ,"Players:"
      ,"  Jack:"
      ,"    password: Black"
      ,"    ships:"
      ,"      'Station/Sweet Home':"
      ,"        position: { x: 100, y: 15}"
      ,"        velocity: { x: 0,   y: 0}"
      ,"        modules:"
      ,"          shipyard: {}"
      ,"          cargo:"
      ,"            metals:    100"
      ,"            ice:       100"
      ,"            silicates: 100"
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
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Jack", "Black")
        .expectSuccess());

  client::Ship ship(m_pRouter);
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Sweet Home", ship));

  client::Shipyard shipyard;
  ASSERT_TRUE(client::FindShipyard(ship, shipyard, "shipyard"));
}

TEST_F(ShipyardTests, GetSpecification)
{
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Jack", "Black")
        .expectSuccess());

  client::Ship ship(m_pRouter);
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Sweet Home", ship));

  client::Shipyard shipyard;
  ASSERT_TRUE(client::FindShipyard(ship, shipyard, "shipyard"));

  client::ShipyardSpecification spec;
  ASSERT_TRUE(shipyard.getSpecification(spec));

  EXPECT_DOUBLE_EQ(5, spec.m_nLaborPerSec);
}

TEST_F(ShipyardTests, BindToCargo)
{
  const std::string sBlueprintName = "Ship/MiningDrone1111";
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Jack", "Black")
        .expectSuccess());

  client::Ship station(m_pRouter);
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Sweet Home", station));

  client::Shipyard shipyard;
  ASSERT_TRUE(client::FindShipyard(station, shipyard, "shipyard"));

  ASSERT_EQ(client::Shipyard::eSuccess, shipyard.bindToCargo("shipyard_cargo"));
  ASSERT_EQ(client::Shipyard::eCargoNotFound, shipyard.bindToCargo("bad_cargo"));
  ASSERT_EQ(client::Shipyard::eSuccess, shipyard.bindToCargo("cargo"));
}

TEST_F(ShipyardTests, BlueprintNotFound)
{
  const std::string sBlueprintName = "Ship/NonExistingShip";
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Jack", "Black")
        .expectSuccess());

  client::Ship station(m_pRouter);
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Sweet Home", station));

  client::Shipyard shipyard;
  ASSERT_TRUE(client::FindShipyard(station, shipyard, "shipyard"));

  ASSERT_EQ(client::Shipyard::eSuccess, shipyard.bindToCargo("shipyard_cargo"));

  ASSERT_EQ(client::Shipyard::eBlueprintNotFound,
            shipyard.startBuilding(sBlueprintName, "Drone #1"));
}

TEST_F(ShipyardTests, NotBoundToCargo)
{
  const std::string sBlueprintName = "Ship/MiningDrone";
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Jack", "Black")
        .expectSuccess());

  client::Ship station(m_pRouter);
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Sweet Home", station));

  client::Shipyard shipyard;
  ASSERT_TRUE(client::FindShipyard(station, shipyard, "shipyard"));

  ASSERT_EQ(client::Shipyard::eCargoNotFound,
            shipyard.startBuilding(sBlueprintName, "Drone #1"));
}

TEST_F(ShipyardTests, BuildSuccessCase)
{
  const std::string sBlueprintName = "Ship/MiningDrone";
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Jack", "Black")
        .expectSuccess());

  client::Ship station(m_pRouter);
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Sweet Home", station));

  client::Shipyard shipyard;
  ASSERT_TRUE(client::FindShipyard(station, shipyard, "shipyard"));

  ASSERT_EQ(client::Shipyard::eSuccess, shipyard.bindToCargo("shipyard_cargo"));

  // Moving to shipyard's cargo all requiered resources:
  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*m_pRootCommutator, storage));

  client::Blueprint blueprint;
  ASSERT_EQ(client::BlueprintsStorage::eSuccess,
            storage.getBlueprint(client::BlueprintName(sBlueprintName), blueprint));

  world::ResourcesArray expenses;
  for (world::ResourceItem const& item : blueprint.m_expenses) {
    expenses[item.m_eType] += item.m_nAmount;
  }
  ASSERT_TRUE(client::ResourcesManagment::shift(
                station, "cargo", "shipyard_cargo", expenses));

  // Start building and wait for confimation
  ASSERT_EQ(client::Shipyard::eBuildStarted,
            shipyard.startBuilding(sBlueprintName, "Drone #1"));

  double      progress;
  uint32_t    nSlotId = 0;
  std::string sShipName;
  ASSERT_EQ(client::Shipyard::eSuccess,
            shipyard.waitingWhileBuilding(&progress, &nSlotId, &sShipName));

  // Connecting to ship, that has been built
  client::Ship drone(m_pRouter);
  {
    client::Router::SessionPtr pTunnel = m_pRootCommutator->openSession(nSlotId);
    ASSERT_TRUE(pTunnel != nullptr);
    drone.attachToChannel(pTunnel);
  }

  // Check that shipyard's container is empty now (all resources has been consumed)
  client::ResourceContainer shipyardContainer;
  client::FindResourceContainer(station, shipyardContainer, "shipyard_cargo");
  world::ResourcesArray empty;
  empty.fill(0);
  ASSERT_TRUE(shipyardContainer.checkContent(empty));

  // Check that new ship has the same position as a shipyard
  pauseTime();
  geometry::Point stationPosition;
  geometry::Vector stationVelocity;
  ASSERT_TRUE(station.getPosition(stationPosition, stationVelocity));

  geometry::Point  dronePosition;
  geometry::Vector droneVelocity;
  ASSERT_TRUE(drone.getPosition(dronePosition, droneVelocity));

  ASSERT_TRUE(dronePosition.almostEqual(stationPosition, 0.1));
  ASSERT_TRUE(droneVelocity.almostEqual(stationVelocity, 0.1));
}

TEST_F(ShipyardTests, BuildFrozen)
{
  const std::string sBlueprintName = "Ship/MiningDrone";
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("Jack", "Black")
        .expectSuccess());

  client::Ship station(m_pRouter);
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Sweet Home", station));

  client::Shipyard shipyard;
  ASSERT_TRUE(client::FindShipyard(station, shipyard, "shipyard"));

  ASSERT_EQ(client::Shipyard::eSuccess, shipyard.bindToCargo("shipyard_cargo"));

  // Moving to shipyard's cargo 40% of total requiered resources (it will be not enough
  // to finish building procedure)
  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*m_pRootCommutator, storage));

  client::Blueprint blueprint;
  ASSERT_EQ(client::BlueprintsStorage::eSuccess,
            storage.getBlueprint(client::BlueprintName(sBlueprintName), blueprint));

  world::ResourcesArray expenses;
  for (world::ResourceItem const& item : blueprint.m_expenses) {
    expenses[item.m_eType] += item.m_nAmount * 0.4; // because 40%
  }
  ASSERT_TRUE(client::ResourcesManagment::shift(
                station, "cargo", "shipyard_cargo", expenses));

  // Start building and wait for eBuildFreezed status, cause run out of resources
  ASSERT_EQ(client::Shipyard::eBuildStarted,
            shipyard.startBuilding(sBlueprintName, "Drone #1"));

  double      progress;
  uint32_t    nSlotId = 0;
  std::string sShipName;
  ASSERT_EQ(client::Shipyard::eBuildFrozen,
            shipyard.waitingWhileBuilding(&progress, &nSlotId, &sShipName));
  ASSERT_NEAR(0.4, progress, 0.05);

  // Put resources, that are requiered to continue building (40% of total)
  expenses.fill(0);
  for (world::ResourceItem const& item : blueprint.m_expenses) {
    expenses[item.m_eType] += item.m_nAmount * 0.4;
  }
  ASSERT_TRUE(client::ResourcesManagment::shift(
                station, "cargo", "shipyard_cargo", expenses));

  shipyard.dropQueuedMessage();
  ASSERT_EQ(client::Shipyard::eBuildFrozen,
            shipyard.waitingWhileBuilding(&progress, &nSlotId, &sShipName));
  ASSERT_NEAR(0.8, progress, 0.05);

  // Rebind shipyard to another container that has enough resources
  // to complete build
  ASSERT_EQ(client::Shipyard::eSuccess, shipyard.bindToCargo("cargo"));

  // Check that build completed
  client::Shipyard::Status eStatus = client::Shipyard::eSuccess;
  utils::Stopwatch watchDog;
  do {
    eStatus = shipyard.waitingWhileBuilding(&progress, &nSlotId, &sShipName);
    ASSERT_NEAR(1.0, progress, 0.001);
    ASSERT_LT(watchDog.testMs(), 500);  // To prevent infinite loop
  } while (eStatus == client::Shipyard::eBuildFrozen);
  ASSERT_EQ(client::Shipyard::eSuccess, eStatus);

}

} // namespace autotests

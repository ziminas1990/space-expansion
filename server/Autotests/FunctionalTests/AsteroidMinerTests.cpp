#include "Autotests/ClientSDK/Modules/ClientCommutator.h"
#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientAsteroidMiner.h>
#include <Autotests/ClientSDK/Modules/ClientResourceContainer.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>
#include <Autotests/ClientSDK/Procedures/Navigation.h>
#include <Utils/Stopwatch.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class AsteroidMinerTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Modules:",
      "    AsteroidMiner:",
      "      ancient-nordic-miner:",
      "        max_distance:     2000",
      "        cycle_time_ms:    1000",
      "        yield_per_cycle:  1000",
      "        expenses:",
      "          labor: 100",
      "    Engine:",
      "      ancient-nordic-engine:",
      "        max_thrust: 50000",
      "        expenses:",
      "          labor: 100",
      "    ResourceContainer:",
      "      ancient-nordic-cargo:",
      "        volume: 10",
      "        expenses:",
      "          labor: 100",
      "  Ships:",
      "    Miner:",
      "      radius: 100",
      "      weight: 100000",
      "      modules:",
      "        miner:  AsteroidMiner/ancient-nordic-miner",
      "        engine: Engine/ancient-nordic-engine",
      "        cargo:  ResourceContainer/ancient-nordic-cargo",
      "        cargo-2:  ResourceContainer/ancient-nordic-cargo",
      "      expenses:",
      "        labor: 1000",
      "Players:",
      "  mega_miner:",
      "    password: unabtainable",
      "    ships:",
      "      'Miner/Miner One':",
      "        position: { x: 0, y: 0}",
      "        velocity: { x: 0, y: 0}",
      "        modules:",
      "          engine: { x: 0, y: 0}",
      "World:",
      "  Asteroids:",
      "    - { position:  { x: 100, y: 100},",
      "        velocity:  { x: 0,   y: 0},",
      "        radius:     10,",
      "        silicates:  5,",
      "        metals:     3,",
      "        ice:        1,"
      "        stones:     25}",
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }
};

TEST_F(AsteroidMinerTests, GetSpecification)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::Ship ship(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));

  client::AsteroidMinerSpecification specification;
  ASSERT_TRUE(miner.getSpecification(specification));
  EXPECT_EQ(2000, specification.m_nMaxDistance);
  EXPECT_EQ(1000, specification.m_nCycleTimeMs);
  EXPECT_EQ(1000, specification.m_nYieldPerCycle);
}

TEST_F(AsteroidMinerTests, BindingToCargo)
{
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::Ship ship(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));

  client::ResourceContainer cargo;
  ASSERT_TRUE(client::FindResourceContainer(ship, cargo, "cargo"));

  ASSERT_EQ(client::AsteroidMiner::eNotBoundToCargo,
            miner.bindToCargo("NonExistingCargo"));

  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.bindToCargo("cargo"));
  // Second attempt should also pass fine
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.bindToCargo("cargo"));

  ASSERT_EQ(client::AsteroidMiner::eNotBoundToCargo,
            miner.bindToCargo("NonExistingCargo"));

  // Try to attach to another cargo
  client::ResourceContainer cargo2;
  ASSERT_TRUE(client::FindResourceContainer(ship, cargo, "cargo-2"));
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.bindToCargo("cargo-2"));
}

TEST_F(AsteroidMinerTests, StartMiningAndWaitReports)
{
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::Ship ship(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));

  client::ResourceContainer cargo;
  ASSERT_TRUE(client::FindResourceContainer(ship, cargo, "cargo"));

  // The miner has not been bint to cargo
  ASSERT_EQ(client::AsteroidMiner::eNotBoundToCargo, miner.startMining(0));

  // Binding mining to the cargo and trying again (should be ok)
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.bindToCargo("cargo"));

  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.startMining(0));

  world::ResourcesArray minedTotal;
  for(size_t i = 0; i < 10; ++i) {
    world::ResourcesArray mined;
    ASSERT_TRUE(miner.waitMiningReport(mined, 1000)) << "On i #" << i;
    minedTotal      += mined;

    client::ResourceContainer::Content content;
    cargo.getContent(content);

    EXPECT_EQ(content.m_amount, minedTotal);
  }
}

TEST_F(AsteroidMinerTests, StopMining)
{
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::Ship ship(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.bindToCargo("cargo"));

  ASSERT_EQ(client::AsteroidMiner::eMinerIsIdle, miner.stopMining());

  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.startMining(0));

  world::ResourcesArray mined;
  utils::Stopwatch stopwatch;
  for(size_t i = 0; i < 5; ++i) {
    ASSERT_TRUE(miner.waitMiningReport(mined)) << "On i #" << i;
  }
  const uint16_t avgReportTime = static_cast<uint16_t>(stopwatch.testMs() / 5);

  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.stopMining());

  // No more reports are expected
  ASSERT_FALSE(miner.waitMiningReport(mined, (avgReportTime + 5) * 2));
}

TEST_F(AsteroidMinerTests, StartMiningFails)
{
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::Ship ship(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.bindToCargo("cargo"));

  ASSERT_EQ(client::AsteroidMiner::eAsteroidDoesntExist, miner.startMining(42));
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.startMining(0));
  ASSERT_EQ(client::AsteroidMiner::eMinerIsBusy, miner.startMining(0));
}

TEST_F(AsteroidMinerTests, NoAvaliableSpace)
{
  resumeTime();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::Ship ship(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.bindToCargo("cargo"));

  client::ResourceContainer cargo;
  ASSERT_TRUE(client::FindResourceContainer(ship, cargo, "cargo"));

  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.startMining(0));

  client::ResourceContainer::Content content;
  cargo.getContent(content);
  while(!utils::AlmostEqual(content.m_nUsedSpace, content.m_nVolume)) {
    world::ResourcesArray mined;
    ASSERT_TRUE(miner.waitMiningReport(mined, 1000));
    cargo.getContent(content);
  }

  client::AsteroidMiner::Status eStatus;
  ASSERT_TRUE(miner.waitMiningIsStoppedInd(eStatus));
  ASSERT_EQ(client::AsteroidMiner::eNoSpaceAvailable, eStatus);
}

} // namespace autotests

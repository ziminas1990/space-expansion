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
      "        container:        cargo",
      "    Engine:",
      "      ancient-nordic-engine:",
      "        maxThrust: 50000",
      "    ResourceContainer:",
      "      ancient-nordic-cargo:",
      "        volume: 10",
      "  Ships:",
      "    Miner:",
      "      radius: 100",
      "      weight: 100000",
      "      modules:",
      "        miner:  AsteroidMiner/ancient-nordic-miner",
      "        engine: Engine/ancient-nordic-engine",
      "        cargo:  ResourceContainer/ancient-nordic-cargo",
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
      "        mettals:    3,",
      "        ice:        1 }",
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

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));

  client::AsteroidMinerSpecification specification;
  ASSERT_TRUE(miner.getSpecification(specification));
  EXPECT_EQ(2000, specification.m_nMaxDistance);
  EXPECT_EQ(1000, specification.m_nCycleTimeMs);
  EXPECT_EQ(1000, specification.m_nYieldPerCycle);
}

TEST_F(AsteroidMinerTests, StartMiningAndWaitReports)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));

  client::ResourceContainer cargo;
  ASSERT_TRUE(client::FindResourceContainer(ship, cargo, "cargo"));

  ASSERT_EQ(client::AsteroidMiner::eSuccess,
            miner.startMining(0, world::Resources::eMettal));

  double nLastCycleAmount = 0;
  double nYieldTotal      = 0;
  for(size_t i = 0; i < 10; ++i) {
    double nAmount = 0;
    ASSERT_TRUE(miner.waitMiningReport(nAmount, 1000)) << "On i #" << i;
    if (i > 0) {
      EXPECT_LT(nAmount, nLastCycleAmount);
    }
    nYieldTotal      += nAmount;
    nLastCycleAmount  = nAmount;

    client::ResourceContainer::Content content;
    cargo.getContent(content);
    EXPECT_DOUBLE_EQ(content.mettals(), nYieldTotal);
  }
}

TEST_F(AsteroidMinerTests, StopMining)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));

  ASSERT_EQ(client::AsteroidMiner::eMinerIsIdle, miner.stopMining());

  ASSERT_EQ(client::AsteroidMiner::eSuccess,
            miner.startMining(0, world::Resources::eMettal));

  utils::Stopwatch stopwatch;
  double nAmount = 0;
  for(size_t i = 0; i < 5; ++i) {
    ASSERT_TRUE(miner.waitMiningReport(nAmount)) << "On i #" << i;
  }
  uint16_t avgReportTime = static_cast<uint16_t>(stopwatch.testMs() / 5);

  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.stopMining());

  // No more reports are expected
  ASSERT_FALSE(miner.waitMiningReport(nAmount, (avgReportTime + 5) * 2));
}

TEST_F(AsteroidMinerTests, MiningVariousResources)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));

  double nTotalMettals   = 0;
  double nTotalSilicates = 0;
  double nTotalIce       = 0;

  ASSERT_EQ(client::AsteroidMiner::eSuccess,
            miner.startMining(0, world::Resources::eMettal));
  for(size_t i = 0; i < 5; ++i) {
    double nAmount = 0;
    ASSERT_TRUE(miner.waitMiningReport(nAmount, 1000)) << "On i #" << i;
    nTotalMettals += nAmount;
  }
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.stopMining());

  ASSERT_EQ(client::AsteroidMiner::eSuccess,
            miner.startMining(0, world::Resources::eSilicate));
  for(size_t i = 0; i < 5; ++i) {
    double nAmount = 0;
    ASSERT_TRUE(miner.waitMiningReport(nAmount, 1000)) << "On i #" << i;
    nTotalSilicates += nAmount;
  }
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.stopMining());

  ASSERT_EQ(client::AsteroidMiner::eSuccess,
            miner.startMining(0, world::Resources::eIce));
  for(size_t i = 0; i < 5; ++i) {
    double nAmount = 0;
    ASSERT_TRUE(miner.waitMiningReport(nAmount, 1000)) << "On i #" << i;
    nTotalIce += nAmount;
  }
  ASSERT_EQ(client::AsteroidMiner::eSuccess, miner.stopMining());

  client::ResourceContainer cargo;
  ASSERT_TRUE(client::FindResourceContainer(ship, cargo, "cargo"));
  client::ResourceContainer::Content content;
  cargo.getContent(content);
  EXPECT_DOUBLE_EQ(content.mettals(),   nTotalMettals);
  EXPECT_DOUBLE_EQ(content.silicates(), nTotalSilicates);
  EXPECT_DOUBLE_EQ(content.ice(),       nTotalIce);
}

TEST_F(AsteroidMinerTests, StartMiningFails)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));

  ASSERT_EQ(client::AsteroidMiner::eAsteroidDoesntExist,
            miner.startMining(42, world::Resources::eMettal));

  ASSERT_EQ(client::AsteroidMiner::eSuccess,
            miner.startMining(0, world::Resources::eMettal));

  ASSERT_EQ(client::AsteroidMiner::eMinerIsBusy,
            miner.startMining(0, world::Resources::eMettal));
}

TEST_F(AsteroidMinerTests, NoAvaliableSpace)
{
  animateWorld();

  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Miner One", ship));

  client::AsteroidMiner miner;
  ASSERT_TRUE(client::FindAsteroidMiner(ship, miner, "miner"));

  client::ResourceContainer cargo;
  ASSERT_TRUE(client::FindResourceContainer(ship, cargo, "cargo"));

  ASSERT_EQ(client::AsteroidMiner::eSuccess,
            miner.startMining(0, world::Resources::eIce));

  client::ResourceContainer::Content content;
  cargo.getContent(content);
  while(content.m_nUsedSpace < content.m_nVolume) {
    double nAmount = 0;
    ASSERT_TRUE(miner.waitMiningReport(nAmount, 1000));
    cargo.getContent(content);
  }

  ASSERT_TRUE(miner.waitError(client::AsteroidMiner::eNoSpaceAvaliable));
}

} // namespace autotests

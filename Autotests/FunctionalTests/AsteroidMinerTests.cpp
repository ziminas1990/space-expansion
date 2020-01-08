#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientAsteroidMiner.h>
#include <Autotests/ClientSDK/Modules/ClientResourceContainer.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>
#include <Autotests/ClientSDK/Procedures/Navigation.h>

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
      "  Miner:",
      "    radius: 100",
      "    weight: 100000",
      "    modules:",
      "      miner:",
      "        type:             AsteroidMiner",
      "        max_distance:     2000",
      "        cycle_time_ms:    1000",
      "        yield_per_cycle:  1000",
      "        container:        cargo",
      "      engine:",
      "        type:      engine",
      "        maxThrust: 50000",
      "      cargo:",
      "        type:   ResourceContainer",
      "        volume: 10",
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
      "        velocity:  { x: 0,    y: 0},",
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


} // namespace autotests

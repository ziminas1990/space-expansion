#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientAsteroidMiner.h>
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
      "    weight: 100000 ",
      "    modules:",
      "      miner:",
      "        type:             AsteroidMiner",
      "        max_distance:     2000",
      "        cycle_time_ms:    50000",
      "        yield_per_cycle:  1000",
      "      engine: ",
      "        type:      engine",
      "        maxThrust: 50000",
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
      "        radius:     100,",
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
  EXPECT_EQ(2000,  specification.m_nMaxDistance);
  EXPECT_EQ(50000, specification.m_nCycleTimeMs);
  EXPECT_EQ(1000,  specification.m_nYieldPerCycle);
}


} // namespace autotests

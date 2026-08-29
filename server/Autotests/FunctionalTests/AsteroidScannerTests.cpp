#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientPassiveScanner.h>
#include <Autotests/ClientSDK/Modules/ClientAsteroidScanner.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>
#include <Autotests/ClientSDK/Procedures/Navigation.h>
#include <World/ObjectTypes.h>

#include <yaml-cpp/yaml.h>
#include <sstream>
#include <set>

namespace autotests
{

class AsteroidScannerTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Modules:",
      "    PassiveScanner:",
      "      ancient-nordic-scanner:",
      "        max_scanning_radius_km: 200",
      "        edge_update_time_ms:     10",
      "        expenses:",
      "          labor: 100",
      "    AsteroidScanner:",
      "      ancient-nordic-scanner:",
      "        max_scanning_distance:  1000",
      "        scanning_time_ms:       100",
      "        expenses:",
      "          labor: 100",
      "    Engine:",
      "      ancient-nordic-engine:",
      "        max_thrust: 500",
      "        expenses:",
      "          labor: 100",
      "  Ships:",
      "    Ancient-Nordic-Miner:",
      "      radius: 0.1",
      "      weight: 10",
      "      modules:",
      "        passive-scanner:  PassiveScanner/ancient-nordic-scanner",
      "        asteroid-scanner: AsteroidScanner/ancient-nordic-scanner",
      "        engine:           Engine/ancient-nordic-engine",
      "      expenses:",
      "        labor: 100",
      "Players:",
      "  mega_miner:",
      "    password: unabtainable",
      "    ships:",
      "      'Ancient-Nordic-Miner/Miner One':",
      "        position: { x: 0, y: 0}",
      "        velocity: { x: 0, y: 0}",
      "        modules:",
      "          engine: { x: 0, y: 0}",
      "World:",
      "  Asteroids:",
      "    - { position:  { x: 100000, y: 0},",
      "        velocity:  { x: 0,    y: 0},",
      "        radius:    100,",
      "        silicates:  80,",
      "        metals:     15,",
      "        ice:         5 }",
      "    - { position:  { x: 0, y: 100000},",
      "        velocity:  { x: 0, y: 0},",
      "        radius:    100,",
      "        silicates:  50,",
      "        metals:      3,",
      "        ice:        40 }"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }

  bool waitAsteroidNear(client::ClientPassiveScanner& scanner,
                        geometry::Point const& near,
                        uint32_t* nAsteroidId)
  {
    for (int i = 0; i < 20; ++i) {
      std::vector<client::ClientPassiveScanner::ObjectData> update;
      if (!scanner.waitUpdate(update)) {
        continue;
      }
      for (auto const& obj : update) {
        if (obj.m_eType == world::ObjectType::eAsteroid
            && obj.m_position.distance(near) < 1000.0) {
          *nAsteroidId = obj.m_nObjectId;
          return true;
        }
      }
    }
    return false;
  }

  bool waitAsteroids(client::ClientPassiveScanner& scanner,
                     size_t nMinCount,
                     std::vector<uint32_t>* ids)
  {
    std::set<uint32_t> seen;
    for (int i = 0; i < 20; ++i) {
      std::vector<client::ClientPassiveScanner::ObjectData> update;
      if (!scanner.waitUpdate(update)) {
        continue;
      }
      for (auto const& obj : update) {
        if (obj.m_eType == world::ObjectType::eAsteroid) {
          seen.insert(obj.m_nObjectId);
        }
      }
      if (seen.size() >= nMinCount) {
        ids->assign(seen.begin(), seen.end());
        return true;
      }
    }
    return false;
  }
};

TEST_F(AsteroidScannerTests, GetSpecification)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::Ship ship(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Miner One", ship));

  client::AsteroidScanner scanner;
  ASSERT_TRUE(client::FindAsteroidScanner(ship, scanner));

  client::AsteroidScannerSpecification specification;
  ASSERT_TRUE(scanner.getSpecification(specification));
  EXPECT_EQ(1000, specification.m_nMaxScanningDistance);
  EXPECT_EQ(100,  specification.m_nProcessingTimeUs);
}

TEST_F(AsteroidScannerTests, SimpleScanningTest)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::ShipPtr pShip = std::make_shared<client::Ship>(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Miner One", *pShip));

  client::AsteroidScanner asteroidScanner;
  ASSERT_TRUE(client::FindAsteroidScanner(*pShip, asteroidScanner));

  client::Navigation navigator(pShip);
  ASSERT_TRUE(navigator.initialize());

  // Moving to first asteroid and scanning it
  {
    pauseTime();
    geometry::Point asteroidPosition(100000, 0);
    ASSERT_TRUE(Scenarios::RunProcedures()
                .add(navigator.MakeMoveToProcedure(asteroidPosition, 100))
                .wait(50, 15000, 25000));

    resumeTime();
    client::ClientPassiveScanner passiveScanner;
    ASSERT_TRUE(client::FindModule(*pShip, "PassiveScanner", passiveScanner));
    ASSERT_TRUE(passiveScanner.sendMonitor());
    ASSERT_TRUE(passiveScanner.waitMonitorAck());
    uint32_t nAsteroidId = 0;
    ASSERT_TRUE(waitAsteroidNear(passiveScanner, asteroidPosition, &nAsteroidId));
    client::AsteroidScanner::AsteroidInfo composition;
    ASSERT_EQ(asteroidScanner.scan(nAsteroidId, &composition),
              client::AsteroidScanner::eSuccess);
    EXPECT_NEAR(composition.m_metalsPercent,    0.15, 0.0001);
    EXPECT_NEAR(composition.m_silicatesPercent, 0.8,  0.0001);
    EXPECT_NEAR(composition.m_icePercent,       0.05, 0.0001);
    passiveScanner.dropQueuedMessage();
    passiveScanner.disconnect();
  }

  {
    pauseTime();
    geometry::Point asteroidPosition(0, 100000);
    ASSERT_TRUE(Scenarios::RunProcedures()
                .add(navigator.MakeMoveToProcedure(asteroidPosition, 100))
                .wait(50, 15000, 25000));

    resumeTime();
    client::ClientPassiveScanner passiveScanner;
    ASSERT_TRUE(client::FindModule(*pShip, "PassiveScanner", passiveScanner));
    ASSERT_TRUE(passiveScanner.sendMonitor());
    ASSERT_TRUE(passiveScanner.waitMonitorAck());
    uint32_t nAsteroidId = 0;
    ASSERT_TRUE(waitAsteroidNear(passiveScanner, asteroidPosition, &nAsteroidId));
    client::AsteroidScanner::AsteroidInfo composition;
    ASSERT_EQ(asteroidScanner.scan(nAsteroidId, &composition),
              client::AsteroidScanner::eSuccess);
    EXPECT_NEAR(composition.m_icePercent,       0.4301, 0.001);
    EXPECT_NEAR(composition.m_metalsPercent,    0.0322, 0.001);
    EXPECT_NEAR(composition.m_silicatesPercent, 0.5376, 0.001);
  }
}

TEST_F(AsteroidScannerTests, FailedToScanTest)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::ShipPtr pShip = std::make_shared<client::Ship>(m_pRouter);
  ASSERT_TRUE(client::attachToShip(pCommutator, "Miner One", *pShip));

  client::AsteroidScanner asteroidScanner;
  ASSERT_TRUE(client::FindAsteroidScanner(*pShip, asteroidScanner));

  client::ClientPassiveScanner passiveScanner;
  ASSERT_TRUE(client::FindModule(*pShip, "PassiveScanner", passiveScanner));

  // Trying to scan asteroids, that are far away
  {
    resumeTime();
    ASSERT_TRUE(passiveScanner.sendMonitor());
    ASSERT_TRUE(passiveScanner.waitMonitorAck());
    std::vector<uint32_t> asteroidIds;
    ASSERT_TRUE(waitAsteroids(passiveScanner, 2, &asteroidIds));
    EXPECT_EQ(2, asteroidIds.size());

    for (uint32_t nAsteroidId : asteroidIds) {
      ASSERT_EQ(asteroidScanner.scan(nAsteroidId),
                client::AsteroidScanner::eAsteroidTooFar);
    }
  }
}

} // namespace autotests

#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientCelestialScanner.h>
#include <Autotests/ClientSDK/Modules/ClientAsteroidScanner.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>
#include <Autotests/ClientSDK/Procedures/Navigation.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

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
      "    CelestialScanner:",
      "      ancient-nordic-scanner:",
      "        max_scanning_radius_km: 200",
      "        processing_time_us:     10",
      "    AsteroidScanner:",
      "      ancient-nordic-scanner:",
      "        max_scanning_distance:  1000",
      "        scanning_time_ms:       100",
      "    Engine:",
      "      ancient-nordic-engine:",
      "        maxThrust: 500",
      "  Ships:",
      "    Ancient-Nordic-Miner:",
      "      radius: 0.1",
      "      weight: 10",
      "      modules:",
      "        celestial-scanner: CelestialScanner/ancient-nordic-scanner",
      "        asteroid-scanner:  AsteroidScanner/ancient-nordic-scanner",
      "        engine:            Engine/ancient-nordic-engine",
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
      "        mettals:    15,",
      "        ice:         5 }",
      "    - { position:  { x: 0, y: 100000},",
      "        velocity:  { x: 0, y: 0},",
      "        radius:    100,",
      "        silicates:  50,",
      "        mettals:     3,",
      "        ice:        40 }"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }
};

TEST_F(AsteroidScannerTests, GetSpecification)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Miner One", ship));

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

  client::ShipPtr pShip = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Miner One", *pShip));

  client::CelestialScanner celestialScanner;
  ASSERT_TRUE(client::FindBestCelestialScanner(*pShip, celestialScanner));

  client::AsteroidScanner asteroidScanner;
  ASSERT_TRUE(client::FindAsteroidScanner(*pShip, asteroidScanner));

  client::Navigation navigator(pShip);
  ASSERT_TRUE(navigator.initialize());

  // Moving to first asteroid and scanning it
  {
    freezeWorld();
    geometry::Point asteroidPosition(100000, 0);
    ASSERT_TRUE(Scenarios::RunProcedures()
                .add(navigator.MakeMoveToProcedure(asteroidPosition, 100))
                .wait(50, 2000, 25000));

    animateWorld();
    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
    ASSERT_TRUE(celestialScanner.scan(1, 5, asteroids));
    EXPECT_EQ(1, asteroids.size());

    uint32_t nAsteroidId = asteroids.front().nId;
    client::AsteroidScanner::AsteroidInfo composition;
    ASSERT_EQ(asteroidScanner.scan(nAsteroidId, &composition),
              client::AsteroidScanner::eStatusOk);
    EXPECT_NEAR(composition.m_metalsPercent,    0.15, 0.0001);
    EXPECT_NEAR(composition.m_silicatesPercent, 0.8,  0.0001);
    EXPECT_NEAR(composition.m_icePercent,       0.05, 0.0001);
  }

  {
    freezeWorld();
    geometry::Point asteroidPosition(0, 100000);
    ASSERT_TRUE(Scenarios::RunProcedures()
                .add(navigator.MakeMoveToProcedure(asteroidPosition, 100))
                .wait(50, 2000, 25000));

    animateWorld();
    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
    ASSERT_TRUE(celestialScanner.scan(1, 5, asteroids));
    EXPECT_EQ(1, asteroids.size());

    uint32_t nAsteroidId = asteroids.front().nId;
    client::AsteroidScanner::AsteroidInfo composition;
    ASSERT_EQ(asteroidScanner.scan(nAsteroidId, &composition),
              client::AsteroidScanner::eStatusOk);
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

  client::ShipPtr pShip = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Miner One", *pShip));

  client::CelestialScanner celestialScanner;
  ASSERT_TRUE(client::FindBestCelestialScanner(*pShip, celestialScanner));

  client::AsteroidScanner asteroidScanner;
  ASSERT_TRUE(client::FindAsteroidScanner(*pShip, asteroidScanner));


  // Trying to scan asteroids, that are far away
  {
    animateWorld();
    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
    ASSERT_TRUE(celestialScanner.scan(200, 5, asteroids));
    EXPECT_EQ(2, asteroids.size());

    for (auto const& asteroidInfo : asteroids) {
      ASSERT_EQ(asteroidScanner.scan(asteroidInfo.nId),
                client::AsteroidScanner::eStatusScanFailed);
    }
  }
}

} // namespace autotests

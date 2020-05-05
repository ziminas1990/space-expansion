#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientCelestialScanner.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>
#include <Autotests/ClientSDK/Procedures/Navigation.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class CelestialScannerTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Modules:",
      "    CelestialScanner:",
      "      ancient-nordic-scanner:",
      "        max_scanning_radius_km: 100000",
      "        processing_time_us:     10",
      "        expenses:",
      "          labor: 1",
      "    Engine:",
      "      ancient-nordic-engine:",
      "        max_thrust: 500",
      "        expenses:",
      "          labor: 1",
      "  Ships:",
      "    Ancient-Nordic-Zond:",
      "      radius: 0.1",
      "      weight: 10",
      "      modules:",
      "        scanner: CelestialScanner/ancient-nordic-scanner",
      "        engine:  Engine/ancient-nordic-engine",
      "      expenses:",
      "        labor: 10",
      "Players:",
      "  mega_miner:",
      "    password: unabtainable",
      "    ships:",
      "      'Ancient-Nordic-Zond/Zorkiy Glaz':",
      "        position: { x: 100000, y: 0}",
      "        velocity: { x: 0,      y: 0}",
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
      "        ice:        40 }",
      // Creating two identical asteroids clouds:
      "  AsteroidsClouds:",
      "    - { pattern:        736628372,",
      "        center:         { x: 100000, y: 0 },",
      "        area_radius_km: 30,",
      "        total:          10 }",
      "    - { pattern:        736628372,",
      "        center:         { x: 0, y: 100000 },",
      "        area_radius_km: 30,",
      "        total:          10 }"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }
};

TEST_F(CelestialScannerTests, GetSpecification)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Zorkiy Glaz", ship));

  client::CelestialScanner scanner;
  ASSERT_TRUE(client::FindBestCelestialScanner(ship, scanner));

  client::CelestialScannerSpecification specification;
  ASSERT_TRUE(scanner.getSpecification(specification));
  EXPECT_EQ(100000, specification.m_nMaxScanningRadiusKm);
  EXPECT_EQ(10,     specification.m_nProcessingTimeUs);
}

TEST_F(CelestialScannerTests, ScanAllAsteroids)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Zorkiy Glaz", ship));

  client::CelestialScanner scanner;
  ASSERT_TRUE(client::FindBestCelestialScanner(ship, scanner));

  resumeTime();
  std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
  ASSERT_TRUE(scanner.scan(1000, 5, asteroids));
  EXPECT_EQ(22, asteroids.size());
}

TEST_F(CelestialScannerTests, ScanAsteroidsNearby)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Zorkiy Glaz", ship));

  client::CelestialScanner scanner;
  ASSERT_TRUE(client::FindBestCelestialScanner(ship, scanner));

  geometry::Point shipPosition;
  ASSERT_TRUE(ship.getPosition(shipPosition));

  resumeTime();

  for (uint32_t nScanRadiusKm = 1; nScanRadiusKm <= 31; ++nScanRadiusKm) {
    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
    ASSERT_TRUE(scanner.scan(nScanRadiusKm, 5, asteroids));

    for(auto asteroid : asteroids)
      EXPECT_LE(shipPosition.distance(asteroid.position), nScanRadiusKm * 1000);

    if (nScanRadiusKm == 31) {
      EXPECT_EQ(11, asteroids.size());
    }
  }
}

TEST_F(CelestialScannerTests, FilteredByAsteroidRadius)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::Ship ship;
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Zorkiy Glaz", ship));

  client::CelestialScanner scanner;
  ASSERT_TRUE(client::FindBestCelestialScanner(ship, scanner));

  resumeTime();

  for (uint32_t nMinimalRadius = 5; nMinimalRadius <= 15; ++nMinimalRadius)
  {
    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
    ASSERT_TRUE(scanner.scan(1000, nMinimalRadius, asteroids));

    for(auto asteroid : asteroids)
      EXPECT_GE(asteroid.radius, nMinimalRadius);

    if (nMinimalRadius == 5) {
      EXPECT_EQ(22, asteroids.size());
    }
  }
}

TEST_F(CelestialScannerTests, ScanningCloudes)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("mega_miner", "unabtainable")
        .expectSuccess());

  client::ShipPtr pShip = std::make_shared<client::Ship>();
  ASSERT_TRUE(client::attachToShip(m_pRootCommutator, "Zorkiy Glaz", *pShip));

  client::CelestialScanner scanner;
  ASSERT_TRUE(client::FindBestCelestialScanner(*pShip, scanner));

  client::Navigation navigator(pShip);
  ASSERT_TRUE(navigator.initialize());

  // Moving to (0, 0) and scanning it (nothing should be scanned)
  {
    pauseTime();
    geometry::Point cloudCenter(0, 0);
    ASSERT_TRUE(Scenarios::RunProcedures()
                .add(navigator.MakeMoveToProcedure(cloudCenter, 100))
                .wait(50, 2000, 25000));

    resumeTime();
    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
    ASSERT_TRUE(scanner.scan(31, 5, asteroids));
    EXPECT_TRUE(asteroids.empty());
  }

  // Moving to first cloud and scanning it
  {
    pauseTime();
    geometry::Point cloudCenter(100000, 0);
    ASSERT_TRUE(Scenarios::RunProcedures()
                .add(navigator.MakeMoveToProcedure(cloudCenter, 100))
                .wait(50, 2000, 25000));

    resumeTime();
    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
    ASSERT_TRUE(scanner.scan(31, 5, asteroids));
    EXPECT_EQ(11, asteroids.size());
  }

  // Moving to second cloud and scanning it
  {
    pauseTime();
    geometry::Point cloudCenter(0, 100000);
    ASSERT_TRUE(Scenarios::RunProcedures()
                .add(navigator.MakeMoveToProcedure(cloudCenter, 100))
                .wait(50, 2000, 25000));

    resumeTime();
    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
    ASSERT_TRUE(scanner.scan(31, 5, asteroids));
    EXPECT_EQ(11, asteroids.size());
  }
}

} // namespace autotests

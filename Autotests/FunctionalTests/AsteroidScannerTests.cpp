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
      "  Miner:",
      "    radius: 0.1",
      "    weight: 10 ",
      "    modules:",
      "      celestial-scanner:",
      "        type:                   CelestialScanner",
      "        max_scanning_radius_km: 100000",
      "        processing_time_us:     10",
      "      asteroid-scanner:",
      "        type:                   AsteroidScanner",
      "        max_scanning_distance:  1000",
      "        scanning_time_ms:       100",
      "      engine: ",
      "        type:      engine",
      "        maxThrust: 500",
      "Players:",
      "  mega_miner:",
      "    password: unabtainable",
      "    ships:",
      "      Miner:",
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

  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
  ASSERT_TRUE(pTunnelToShip);

  client::Ship ship;
  ship.attachToChannel(pTunnelToShip);

  client::AsteroidScanner scanner;
  ASSERT_TRUE(client::FindSomeAsteroidScanner(ship, scanner));

  client::AsteroidScannerSpecification specification;
  ASSERT_TRUE(scanner.getSpecification(specification));
  EXPECT_EQ(1000, specification.m_nMaxScanningDistance);
  EXPECT_EQ(100,  specification.m_nProcessingTimeUs);
}

//TEST_F(CelestialScannerTests, ScanAllAsteroids)
//{
//  ASSERT_TRUE(
//        Scenarios::Login()
//        .sendLoginRequest("mega_miner", "unabtainable")
//        .expectSuccess());

//  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
//  ASSERT_TRUE(pTunnelToShip);

//  client::Ship ship;
//  ship.attachToChannel(pTunnelToShip);

//  client::CelestialScanner scanner;
//  ASSERT_TRUE(client::FindBestCelestialScanner(ship, scanner));

//  animateWorld();
//  std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
//  ASSERT_TRUE(scanner.scan(1000, 5, asteroids));
//  EXPECT_EQ(22, asteroids.size());
//}

//TEST_F(CelestialScannerTests, ScanAsteroidsNearby)
//{
//  ASSERT_TRUE(
//        Scenarios::Login()
//        .sendLoginRequest("mega_miner", "unabtainable")
//        .expectSuccess());

//  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
//  ASSERT_TRUE(pTunnelToShip);

//  client::Ship ship;
//  ship.attachToChannel(pTunnelToShip);

//  client::CelestialScanner scanner;
//  ASSERT_TRUE(client::FindBestCelestialScanner(ship, scanner));

//  geometry::Point shipPosition;
//  ASSERT_TRUE(ship.getPosition(shipPosition));

//  animateWorld();

//  for (uint32_t nScanRadiusKm = 1; nScanRadiusKm <= 31; ++nScanRadiusKm) {
//    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
//    ASSERT_TRUE(scanner.scan(nScanRadiusKm, 5, asteroids));

//    for(auto asteroid : asteroids)
//      EXPECT_LE(shipPosition.distance(asteroid.position), nScanRadiusKm * 1000);

//    if (nScanRadiusKm == 31) {
//      EXPECT_EQ(11, asteroids.size());
//    }
//  }
//}

//TEST_F(CelestialScannerTests, FilteredByAsteroidRadius)
//{
//  ASSERT_TRUE(
//        Scenarios::Login()
//        .sendLoginRequest("mega_miner", "unabtainable")
//        .expectSuccess());

//  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
//  ASSERT_TRUE(pTunnelToShip);

//  client::Ship ship;
//  ship.attachToChannel(pTunnelToShip);

//  client::CelestialScanner scanner;
//  ASSERT_TRUE(client::FindBestCelestialScanner(ship, scanner));

//  animateWorld();

//  for (uint32_t nMinimalRadius = 5; nMinimalRadius <= 15; ++nMinimalRadius)
//  {
//    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
//    ASSERT_TRUE(scanner.scan(1000, nMinimalRadius, asteroids));

//    for(auto asteroid : asteroids)
//      EXPECT_GE(asteroid.radius, nMinimalRadius);

//    if (nMinimalRadius == 5) {
//      EXPECT_EQ(22, asteroids.size());
//    }
//  }
//}

//TEST_F(CelestialScannerTests, ScanningCloudes)
//{
//  ASSERT_TRUE(
//        Scenarios::Login()
//        .sendLoginRequest("mega_miner", "unabtainable")
//        .expectSuccess());

//  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
//  ASSERT_TRUE(pTunnelToShip);

//  client::ShipPtr pShip = std::make_shared<client::Ship>();
//  pShip->attachToChannel(pTunnelToShip);

//  client::CelestialScanner scanner;
//  ASSERT_TRUE(client::FindBestCelestialScanner(*pShip, scanner));

//  client::Navigation navigator(pShip);
//  ASSERT_TRUE(navigator.initialize());

//  // Moving to (0, 0) and scanning it (nothing should be scanned)
//  {
//    freezeWorld();
//    geometry::Point cloudCenter(0, 0);
//    ASSERT_TRUE(Scenarios::RunProcedures()
//                .add(navigator.MakeMoveToProcedure(cloudCenter, 100))
//                .wait(50, 2000, 25000));

//    animateWorld();
//    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
//    ASSERT_TRUE(scanner.scan(31, 5, asteroids));
//    EXPECT_TRUE(asteroids.empty());
//  }

//  // Moving to first cloud and scanning it
//  {
//    freezeWorld();
//    geometry::Point cloudCenter(100000, 0);
//    ASSERT_TRUE(Scenarios::RunProcedures()
//                .add(navigator.MakeMoveToProcedure(cloudCenter, 100))
//                .wait(50, 2000, 25000));

//    animateWorld();
//    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
//    ASSERT_TRUE(scanner.scan(31, 5, asteroids));
//    EXPECT_EQ(11, asteroids.size());
//  }

//  // Moving to second cloud and scanning it
//  {
//    freezeWorld();
//    geometry::Point cloudCenter(0, 100000);
//    ASSERT_TRUE(Scenarios::RunProcedures()
//                .add(navigator.MakeMoveToProcedure(cloudCenter, 100))
//                .wait(50, 2000, 25000));

//    animateWorld();
//    std::vector<client::CelestialScanner::AsteroidInfo> asteroids;
//    ASSERT_TRUE(scanner.scan(31, 5, asteroids));
//    EXPECT_EQ(11, asteroids.size());
//  }
//}

} // namespace autotests

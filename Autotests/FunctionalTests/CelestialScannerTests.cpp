#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientCelestialScanner.h>

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
      "  Zond:",
      "    radius: 0.1",
      "    weight: 10 ",
      "    modules:",
      "      scanner:",
      "        type:                   CelestialScanner",
      "        max_scanning_radius_km: 100000",
      "        processing_time_us:     10",
      "      engine: ",
      "        type:      engine",
      "        maxThrust: 2000",
      "Players:",
      "  mega_miner:",
      "    password: unabtainable",
      "    ships:",
      "      Zond:",
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
      "        mettals:    15,",
      "        ice:         5 }",
      "    - { position:  { x: 0, y: 100000},",
      "        velocity:  { x: 0, y: 0},",
      "        radius:    100,",
      "        silicates:  50,",
      "        mettals:     3,",
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

  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
  ASSERT_TRUE(pTunnelToShip);

  client::Ship ship;
  ship.attachToChannel(pTunnelToShip);

  client::CelestialScanner scanner;
  scanner.attachToChannel(ship.openTunnel(1));

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

  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
  ASSERT_TRUE(pTunnelToShip);

  client::Ship ship;
  ship.attachToChannel(pTunnelToShip);

  client::CelestialScanner scanner;
  scanner.attachToChannel(ship.openTunnel(1));

  animateWorld();
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

  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
  ASSERT_TRUE(pTunnelToShip);

  client::Ship ship;
  ship.attachToChannel(pTunnelToShip);

  client::CelestialScanner scanner;
  scanner.attachToChannel(ship.openTunnel(1));

  geometry::Point shipPosition;
  ASSERT_TRUE(ship.getPosition(shipPosition));

  animateWorld();

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

  client::TunnelPtr pTunnelToShip = m_pRootCommutator->openTunnel(0);
  ASSERT_TRUE(pTunnelToShip);

  client::Ship ship;
  ship.attachToChannel(pTunnelToShip);

  client::CelestialScanner scanner;
  scanner.attachToChannel(ship.openTunnel(1));

  animateWorld();

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

} // namespace autotests

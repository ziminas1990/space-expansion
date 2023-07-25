#include <Autotests/Modules/ModulesTestFixture.h>

#include <Modules/Engine/Engine.h>
#include <Autotests/Modules/Helper.h>

namespace autotests {

class EngineTests : public ModulesTestFixture
{};

TEST_F(EngineTests, GetSpecification)
{
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));

  client::EngineSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));
  ASSERT_EQ(nMaxThrust, spec.nMaxThrust);
}

TEST_F(EngineTests, SetAndGetThrust)
{
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));

  const geometry::Vector thrust = geometry::Vector(1, 2).ofLength(100);
  ASSERT_TRUE(engine->setThrust(thrust, 100));

  geometry::Vector currentThrust;
  ASSERT_TRUE(engine->getThrust(currentThrust));

  EXPECT_EQ(thrust, currentThrust);
}

TEST_F(EngineTests, SetThrustExceedsMaxThrust)
{
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));

  geometry::Vector thrustDirection = geometry::Vector(10, 20).normalized();

  const geometry::Vector thrust = thrustDirection.ofLength(2 * nMaxThrust);
  ASSERT_TRUE(engine->setThrust(thrust, 100));

  geometry::Vector currentThrust;
  ASSERT_TRUE(engine->getThrust(currentThrust));

  EXPECT_EQ(thrustDirection.ofLength(nMaxThrust), currentThrust);
}

TEST_F(EngineTests, MovingWithEngine)
{
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 10000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));

  const geometry::Point startPosition(100, 15);
  ship.m_pRemote->moveTo(startPosition);

  const uint32_t nTotalBurnTimeMs = 100000; // 100 sec
  const geometry::Vector thrust = geometry::Vector(23, 34).ofLength(nMaxThrust);
  ASSERT_TRUE(engine->setThrust(thrust, nTotalBurnTimeMs));
  proceedEnviroment();  // Let the engine to handle the request

  const geometry::Vector expectedAcc = thrust / ship.m_pRemote->getWeight();

  const auto startTime  = utils::GlobalClock::now();
  uint32_t timePassedUs = 0;
  while (timePassedUs < nTotalBurnTimeMs * 1000) {
    timePassedUs = utils::GlobalClock::now() - startTime;
    const double           nBurnTimeSec     = timePassedUs / 1000000.0;
    const geometry::Point  currentPosition  = ship.m_pRemote->getPosition();
    const geometry::Vector currentVelocity  = ship.m_pRemote->getVelocity();
    const geometry::Vector dv               = expectedAcc * nBurnTimeSec;
    const geometry::Point  expectedPosition =
                                        startPosition + dv * nBurnTimeSec * 0.5;

    EXPECT_TRUE(currentPosition.almostEqual(expectedPosition, 0.01));
    EXPECT_TRUE(currentVelocity.almostEqual(dv, 0.01));
    proceedEnviroment();
  }

  // Engine should be switched off now
  const auto             stopTime     = utils::GlobalClock::now();
  const geometry::Point  endPosition  = ship.m_pRemote->getPosition();
  const geometry::Vector endVelocity  = ship.m_pRemote->getVelocity();
  while (timePassedUs < nTotalBurnTimeMs * 1000) {
    timePassedUs = utils::GlobalClock::now() - stopTime;
    const double           nTimePassedSec   = timePassedUs / 1000000.0;
    const geometry::Point  currentPosition  = ship.m_pRemote->getPosition();
    const geometry::Vector currentVelocity  = ship.m_pRemote->getVelocity();
    const geometry::Point  expectedPosition =
                                     endPosition + endVelocity * nTimePassedSec;
    EXPECT_TRUE(currentPosition.almostEqual(expectedPosition, 0.01));
    EXPECT_TRUE(currentVelocity.almostEqual(endVelocity, 0.01));
    proceedEnviroment();
  }
}

}  // namespace autotests
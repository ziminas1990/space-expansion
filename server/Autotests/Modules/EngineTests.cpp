#include <Autotests/Modules/ModulesTestFixture.h>

#include <Modules/Engine/Engine.h>
#include <Autotests/Modules/Helper.h>

namespace autotests {

namespace {

client::EnginePtr openEngineSession(ShipBinding& ship, uint32_t nSlotId)
{
    client::EnginePtr pSession = std::make_shared<client::Engine>();
    pSession->attachToChannel(ship->openSession(nSlotId));
    return pSession;
}

}  // namespace

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

TEST_F(EngineTests, MonitorSnapshotAndAppliedThrust)
{
  // 1. connect and spawn a ship with an engine
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

  // 2. subscribe to thrust monitoring
  client::EnginePtr pMonitor = openEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));
  EXPECT_EQ(geometry::Vector(), snapshot);

  // 3. apply a new thrust vector
  const geometry::Vector thrust = geometry::Vector(1, 2).ofLength(100);
  ASSERT_TRUE(engine->setThrust(thrust, 10000));

  // 4. monitor receives the applied vector
  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(thrust, applied);

  // 5. change_thrust session has no reply
  spex::IEngine unexpected;
  ASSERT_FALSE(engine->pick(unexpected));
}

TEST_F(EngineTests, MonitorClampedThrust)
{
  // 1. connect and spawn a ship with an engine
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

  // 2. subscribe to thrust monitoring
  client::EnginePtr pMonitor = openEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. request a thrust above max_thrust
  geometry::Vector thrustDirection = geometry::Vector(10, 20).normalized();
  ASSERT_TRUE(engine->setThrust(thrustDirection.ofLength(2 * nMaxThrust), 10000));

  // 4. monitor receives the clamped vector
  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(thrustDirection.ofLength(nMaxThrust), applied);
}

TEST_F(EngineTests, MonitorDurationExpiry)
{
  // 1. connect and spawn a ship with an engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(100000));

  // 2. subscribe to thrust monitoring
  client::EnginePtr pMonitor = openEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. start a short burn
  const geometry::Vector thrust = geometry::Vector(1, 0).ofLength(100);
  ASSERT_TRUE(engine->setThrust(thrust, 200));

  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(thrust, applied);

  // 4. wait for the burn to expire
  geometry::Vector expired;
  ASSERT_TRUE(pMonitor->waitThrust(expired, 500));
  EXPECT_EQ(geometry::Vector(), expired);
}

TEST_F(EngineTests, MonitorNotifiesRepeatedThrust)
{
  // 1. connect and spawn a ship with an engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(100000));

  // 2. subscribe to thrust monitoring
  client::EnginePtr pMonitor = openEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. apply a thrust vector
  const geometry::Vector thrust = geometry::Vector(3, 4).ofLength(100);
  ASSERT_TRUE(engine->setThrust(thrust, 1000000));

  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(thrust, applied);

  // 4. send the same command again
  ASSERT_TRUE(engine->setThrust(thrust, 1000000));

  // 5. monitor receives another indication
  geometry::Vector repeated;
  ASSERT_TRUE(pMonitor->waitThrust(repeated));
  EXPECT_EQ(thrust, repeated);
}

TEST_F(EngineTests, MonitorDelayedThrust)
{
  // 1. connect and spawn a ship with an engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(100000));

  // 2. subscribe to thrust monitoring
  client::EnginePtr pMonitor = openEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. schedule a delayed change_thrust
  const geometry::Vector thrust = geometry::Vector(0, 1).ofLength(250);
  const uint64_t nWhenUs = utils::GlobalClock::now() + 200000;
  ASSERT_TRUE(engine->setThrust(thrust, 1000000, nWhenUs));

  // 4. monitor does not notify before the timestamp
  while (utils::GlobalClock::now() + 20000 < nWhenUs) {
    proceedEnviroment();
    spex::IEngine tooEarly;
    ASSERT_FALSE(pMonitor->pick(tooEarly));
  }

  // 5. after the timestamp, monitor receives the applied vector
  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied, 500));
  EXPECT_EQ(thrust, applied);
}

TEST_F(EngineTests, SeveralSessionsMayMonitor)
{
  // 1. connect and spawn a ship with an engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(100000));

  // 2. open two monitoring sessions
  client::EnginePtr pFirst = openEngineSession(ship, engine.m_nSlotId);
  client::EnginePtr pSecond = openEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pFirst);
  ASSERT_TRUE(pSecond);

  geometry::Vector firstSnapshot;
  geometry::Vector secondSnapshot;
  ASSERT_TRUE(pFirst->monitor(firstSnapshot));
  ASSERT_TRUE(pSecond->monitor(secondSnapshot));

  // 3. apply thrust and both sessions receive it
  const geometry::Vector thrust = geometry::Vector(5, 12).ofLength(130);
  ASSERT_TRUE(engine->setThrust(thrust, 10000));

  geometry::Vector firstApplied;
  geometry::Vector secondApplied;
  ASSERT_TRUE(pFirst->waitThrust(firstApplied));
  ASSERT_TRUE(pSecond->waitThrust(secondApplied));
  EXPECT_EQ(thrust, firstApplied);
  EXPECT_EQ(thrust, secondApplied);

  // 4. closing the first session does not stop the second
  ASSERT_TRUE(pFirst->disconnect());

  const geometry::Vector nextThrust = geometry::Vector(1, 0).ofLength(80);
  ASSERT_TRUE(engine->setThrust(nextThrust, 10000));

  geometry::Vector secondNext;
  ASSERT_TRUE(pSecond->waitThrust(secondNext));
  EXPECT_EQ(nextThrust, secondNext);
}

}  // namespace autotests
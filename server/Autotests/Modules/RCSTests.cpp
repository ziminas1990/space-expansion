#include <Autotests/Modules/ModulesTestFixture.h>

#include <Modules/RCS/RCS.h>
#include <Autotests/Modules/Helper.h>

namespace autotests {

namespace {

client::RCSPtr openRCSSession(ShipBinding& ship, uint32_t nSlotId)
{
    client::RCSPtr pSession = std::make_shared<client::RCS>();
    pSession->attachToChannel(ship->openSession(nSlotId));
    return pSession;
}

}  // namespace

class RCSTests : public ModulesTestFixture
{};

TEST_F(RCSTests, GetSpecification)
{
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  client::RCSSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));
  ASSERT_EQ(nMaxThrust, spec.nMaxThrust);
}

TEST_F(RCSTests, SetAndGetThrust)
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
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  const geometry::Vector direction(1, 2);
  const geometry::Vector expected = direction.ofLength(nMaxThrust);

  // 2. a direction is applied at maximum thrust
  ASSERT_TRUE(engine->setThrust(direction, 100));

  geometry::Vector currentThrust;
  ASSERT_TRUE(engine->getThrust(currentThrust));
  EXPECT_EQ(expected, currentThrust);

  // 3. a longer vector in the same direction does not change the thrust
  ASSERT_TRUE(engine->setThrust(direction.ofLength(nMaxThrust * 2), 100));
  ASSERT_TRUE(engine->getThrust(currentThrust));
  EXPECT_EQ(expected, currentThrust);

  // 4. a shorter vector in the same direction does not change the thrust
  ASSERT_TRUE(engine->setThrust(direction.ofLength(1), 100));
  ASSERT_TRUE(engine->getThrust(currentThrust));
  EXPECT_EQ(expected, currentThrust);
}

TEST_F(RCSTests, VectorLengthDoesNotChangeAcceleration)
{
  // 1. connect and spawn a ship with an engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 10000;
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  const uint32_t nBurnTimeMs = 1000;
  const geometry::Vector direction(3, 4);
  const geometry::Vector expectedAcc =
      direction.ofLength(nMaxThrust) / ship.m_pRemote->getWeight();

  // 2. a short vector accelerates the ship at maximum thrust
  ship.m_pRemote->moveTo(geometry::Point(0, 0));
  ship.m_pRemote->setVelocity(geometry::Vector());
  ASSERT_TRUE(engine->setThrust(direction.ofLength(1), nBurnTimeMs));
  proceedEnviroment();
  const auto shortStart = utils::GlobalClock::now();
  justWait(nBurnTimeMs / 2);
  const double shortSec =
      (utils::GlobalClock::now() - shortStart) / 1000000.0;
  const geometry::Vector shortVelocity = ship.m_pRemote->getVelocity();
  EXPECT_TRUE(shortVelocity.almostEqual(expectedAcc * shortSec, 0.01));

  // 3. a longer vector in the same direction gives the same acceleration
  ASSERT_TRUE(engine->setThrust(geometry::Vector(), 0));
  proceedEnviroment();
  ship.m_pRemote->moveTo(geometry::Point(0, 0));
  ship.m_pRemote->setVelocity(geometry::Vector());
  ASSERT_TRUE(engine->setThrust(
      direction.ofLength(nMaxThrust * 3), nBurnTimeMs));
  proceedEnviroment();
  const auto longStart = utils::GlobalClock::now();
  justWait(nBurnTimeMs / 2);
  const double longSec = (utils::GlobalClock::now() - longStart) / 1000000.0;
  const geometry::Vector longVelocity = ship.m_pRemote->getVelocity();
  EXPECT_TRUE(longVelocity.almostEqual(expectedAcc * longSec, 0.01));
  EXPECT_TRUE((shortVelocity / shortSec).almostEqual(
      longVelocity / longSec, 0.01));
}

TEST_F(RCSTests, ZeroVectorProducesNoAcceleration)
{
  // 1. connect and spawn a ship with an engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(100000));

  // 2. a zero vector reports no thrust
  ASSERT_TRUE(engine->setThrust(geometry::Vector(), 10000));

  geometry::Vector currentThrust;
  ASSERT_TRUE(engine->getThrust(currentThrust));
  EXPECT_EQ(geometry::Vector(), currentThrust);

  // 3. the ship does not accelerate
  const geometry::Point start = ship.m_pRemote->getPosition();
  proceedEnviroment();
  justWait(200);
  EXPECT_TRUE(ship.m_pRemote->getPosition().almostEqual(start, 0.01));
  EXPECT_TRUE(
      ship.m_pRemote->getVelocity().almostEqual(geometry::Vector(), 0.01));
}

TEST_F(RCSTests, MovingWithRCS)
{
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 10000;
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

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

  // RCS should be switched off now
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

TEST_F(RCSTests, MonitorSnapshotAndAppliedThrust)
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
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  // 2. subscribe to thrust monitoring
  client::RCSPtr pMonitor = openRCSSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));
  EXPECT_EQ(geometry::Vector(), snapshot);

  // 3. apply a new thrust direction
  const geometry::Vector direction(1, 2);
  ASSERT_TRUE(engine->setThrust(direction, 10000));

  // 4. monitor receives maximum thrust along that direction
  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(direction.ofLength(nMaxThrust), applied);

  // 5. change_thrust session has no reply
  spex::IRCS unexpected;
  ASSERT_FALSE(engine->pick(unexpected));
}

TEST_F(RCSTests, MonitorIgnoresVectorLength)
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
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  // 2. subscribe to thrust monitoring
  client::RCSPtr pMonitor = openRCSSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. a long vector is still maximum thrust
  geometry::Vector thrustDirection = geometry::Vector(10, 20).normalized();
  ASSERT_TRUE(engine->setThrust(thrustDirection.ofLength(2 * nMaxThrust), 10000));

  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(thrustDirection.ofLength(nMaxThrust), applied);

  // 4. a short vector in the same direction reports the same thrust
  ASSERT_TRUE(engine->setThrust(thrustDirection.ofLength(1), 10000));
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(thrustDirection.ofLength(nMaxThrust), applied);
}

TEST_F(RCSTests, MonitorDurationExpiry)
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
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  // 2. subscribe to thrust monitoring
  client::RCSPtr pMonitor = openRCSSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. start a short burn
  const geometry::Vector direction(1, 0);
  ASSERT_TRUE(engine->setThrust(direction, 200));

  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(direction.ofLength(nMaxThrust), applied);

  // 4. wait for the burn to expire
  geometry::Vector expired;
  ASSERT_TRUE(pMonitor->waitThrust(expired, 500));
  EXPECT_EQ(geometry::Vector(), expired);
}

TEST_F(RCSTests, MonitorNotifiesRepeatedThrust)
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
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  // 2. subscribe to thrust monitoring
  client::RCSPtr pMonitor = openRCSSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. apply a thrust direction
  const geometry::Vector direction = geometry::Vector(3, 4).ofLength(100);
  ASSERT_TRUE(engine->setThrust(direction, 1000000));

  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(direction.ofLength(nMaxThrust), applied);

  // 4. send the same command again
  ASSERT_TRUE(engine->setThrust(direction, 1000000));

  // 5. monitor receives another indication
  geometry::Vector repeated;
  ASSERT_TRUE(pMonitor->waitThrust(repeated));
  EXPECT_EQ(direction.ofLength(nMaxThrust), repeated);
}

TEST_F(RCSTests, MonitorDelayedThrust)
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
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  // 2. subscribe to thrust monitoring
  client::RCSPtr pMonitor = openRCSSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  geometry::Vector snapshot;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. schedule a delayed change_thrust
  const geometry::Vector direction = geometry::Vector(0, 1).ofLength(250);
  const uint64_t nWhenUs = utils::GlobalClock::now() + 200000;
  ASSERT_TRUE(engine->setThrust(direction, 1000000, nWhenUs));

  // 4. monitor does not notify before the timestamp
  while (utils::GlobalClock::now() + 20000 < nWhenUs) {
    proceedEnviroment();
    spex::IRCS tooEarly;
    ASSERT_FALSE(pMonitor->pick(tooEarly));
  }

  // 5. after the timestamp, monitor receives the applied vector
  geometry::Vector applied;
  ASSERT_TRUE(pMonitor->waitThrust(applied, 500));
  EXPECT_EQ(direction.ofLength(nMaxThrust), applied);
}

TEST_F(RCSTests, SeveralSessionsMayMonitor)
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
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  // 2. open two monitoring sessions
  client::RCSPtr pFirst = openRCSSession(ship, engine.m_nSlotId);
  client::RCSPtr pSecond = openRCSSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pFirst);
  ASSERT_TRUE(pSecond);

  geometry::Vector firstSnapshot;
  geometry::Vector secondSnapshot;
  ASSERT_TRUE(pFirst->monitor(firstSnapshot));
  ASSERT_TRUE(pSecond->monitor(secondSnapshot));

  // 3. apply thrust and both sessions receive it
  const geometry::Vector direction = geometry::Vector(5, 12).ofLength(130);
  ASSERT_TRUE(engine->setThrust(direction, 10000));

  geometry::Vector firstApplied;
  geometry::Vector secondApplied;
  ASSERT_TRUE(pFirst->waitThrust(firstApplied));
  ASSERT_TRUE(pSecond->waitThrust(secondApplied));
  EXPECT_EQ(direction.ofLength(nMaxThrust), firstApplied);
  EXPECT_EQ(direction.ofLength(nMaxThrust), secondApplied);

  // 4. closing the first session does not stop the second
  ASSERT_TRUE(pFirst->disconnect());

  const geometry::Vector nextDirection = geometry::Vector(1, 0).ofLength(80);
  ASSERT_TRUE(engine->setThrust(nextDirection, 10000));

  geometry::Vector secondNext;
  ASSERT_TRUE(pSecond->waitThrust(secondNext));
  EXPECT_EQ(nextDirection.ofLength(nMaxThrust), secondNext);
}

}  // namespace autotests
#include <Autotests/Modules/ModulesTestFixture.h>
#include <Autotests/Modules/Helper.h>

namespace autotests {

namespace {

// One quarter turn, in radians.
constexpr double kQuarterTurn = 1.5707963267948966;
constexpr uint32_t kLongBurnMs = 100000;

client::HoverEnginePtr openHoverEngineSession(ShipBinding& ship, uint32_t nSlotId)
{
  client::HoverEnginePtr pSession = std::make_shared<client::HoverEngine>();
  pSession->attachToChannel(ship->openSession(nSlotId));
  return pSession;
}

}  // namespace


class HoverEngineTests : public ModulesTestFixture
{};

TEST_F(HoverEngineTests, GetSpecification)
{
  // 1. connect and spawn a ship carrying a hover engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  HoverEngineBinding engine = Helper::spawnHoverEngine(
      ship, Helper::HoverEngineParams().maxThrust(nMaxThrust));

  // 2. the module is a HoverEngine and its limit is the maximum thrust
  client::ModuleInfo info;
  ASSERT_TRUE(ship->getModuleInfo(engine.m_nSlotId, info));
  EXPECT_EQ("HoverEngine", info.sModuleType);

  client::HoverEngineSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));
  EXPECT_EQ(nMaxThrust, spec.nMaxThrust);
}

TEST_F(HoverEngineTests, SetAndGetThrust)
{
  // 1. connect and spawn a ship with a hover engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  HoverEngineBinding engine = Helper::spawnHoverEngine(
      ship, Helper::HoverEngineParams().maxThrust(nMaxThrust));

  // 2. the commanded magnitude is the thrust that is applied
  const uint32_t nThrust = 100;
  ASSERT_TRUE(engine->setThrust(nThrust, kLongBurnMs));
  proceedEnviroment();

  uint32_t applied = 1;
  ASSERT_TRUE(engine->getThrust(applied));
  EXPECT_EQ(nThrust, applied);

  // 3. the thrust stays until another command, including zero
  justWait(500);
  ASSERT_TRUE(engine->getThrust(applied));
  EXPECT_EQ(nThrust, applied);

  ASSERT_TRUE(engine->setThrust(0, 0));
  proceedEnviroment();
  ASSERT_TRUE(engine->getThrust(applied));
  EXPECT_EQ(0u, applied);
}

TEST_F(HoverEngineTests, ThrustDoesNotExceedMaximum)
{
  // 1. connect and spawn a ship with a hover engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  const double nMass = 200000;
  ShipBinding ship = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0),
      Helper::ShipParams().weight(nMass));

  const uint32_t nMaxThrust = 10000;
  HoverEngineBinding engine = Helper::spawnHoverEngine(
      ship, Helper::HoverEngineParams().maxThrust(nMaxThrust));

  // 2. a request above the maximum is applied at the maximum
  ASSERT_TRUE(engine->setThrust(nMaxThrust * 2, kLongBurnMs));
  proceedEnviroment();

  uint32_t applied = 0;
  ASSERT_TRUE(engine->getThrust(applied));
  EXPECT_EQ(nMaxThrust, applied);

  // 3. the acceleration matches that clamped thrust and the ship's mass
  const geometry::Vector expectedAcc =
      ship.m_pRemote->getOrientation().ofLength(nMaxThrust) / nMass;
  const uint64_t t0 = utils::GlobalClock::now();
  const geometry::Vector v0 = ship.m_pRemote->getVelocity();
  justWait(1000);
  const double dt = (utils::GlobalClock::now() - t0) / 1000000.0;
  const geometry::Vector velocity = ship.m_pRemote->getVelocity();
  EXPECT_TRUE(velocity.almostEqual(v0 + expectedAcc * dt, 1e-4));
}

TEST_F(HoverEngineTests, AcceleratesAlongNose)
{
  // 1. connect and spawn a ship with a hover engine, nose along +X
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  const double nMass = 200000;
  ShipBinding ship = Helper::spawnShip(
      *this, pCommutator, geometry::Point(100, 15),
      Helper::ShipParams().weight(nMass));
  ASSERT_TRUE(ship.m_pRemote->getOrientation().almostEqual(
      geometry::Vector(1, 0), 1e-9));

  const uint32_t nThrust = 10000;
  HoverEngineBinding engine = Helper::spawnHoverEngine(
      ship, Helper::HoverEngineParams().maxThrust(nThrust));

  // 2. set a thrust and let the engine apply it
  ASSERT_TRUE(engine->setThrust(nThrust, kLongBurnMs));
  proceedEnviroment();

  // 3. the ship accelerates along its nose at thrust / mass
  const geometry::Vector expectedAcc =
      ship.m_pRemote->getOrientation().ofLength(nThrust) / nMass;
  const uint64_t t0 = utils::GlobalClock::now();
  const geometry::Vector v0 = ship.m_pRemote->getVelocity();
  const geometry::Point p0 = ship.m_pRemote->getPosition();
  justWait(1000);
  const double dt = (utils::GlobalClock::now() - t0) / 1000000.0;
  const geometry::Vector dv = expectedAcc * dt;
  EXPECT_TRUE(ship.m_pRemote->getVelocity().almostEqual(v0 + dv, 1e-4));
  EXPECT_TRUE(ship.m_pRemote->getPosition().almostEqual(
      p0 + (v0 + dv * 0.5) * dt, 1e-3));

  // 4. setting the thrust to zero stops that acceleration
  ASSERT_TRUE(engine->setThrust(0, 0));
  proceedEnviroment();
  const geometry::Vector coastVelocity = ship.m_pRemote->getVelocity();
  const geometry::Point coastPosition = ship.m_pRemote->getPosition();
  const uint64_t tCoast = utils::GlobalClock::now();
  justWait(1000);
  const double coastDt = (utils::GlobalClock::now() - tCoast) / 1000000.0;
  EXPECT_TRUE(ship.m_pRemote->getVelocity().almostEqual(coastVelocity, 1e-6));
  EXPECT_TRUE(ship.m_pRemote->getPosition().almostEqual(
      coastPosition + coastVelocity * coastDt, 1e-3));
}

TEST_F(HoverEngineTests, ThrustFollowsNose)
{
  // 1. connect and spawn a ship with a hover engine, nose along +X
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  const double nMass = 200000;
  ShipBinding ship = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0),
      Helper::ShipParams().weight(nMass));

  const uint32_t nThrust = 10000;
  HoverEngineBinding engine = Helper::spawnHoverEngine(
      ship, Helper::HoverEngineParams().maxThrust(nThrust));

  // 2. set a thrust along the current nose
  ASSERT_TRUE(engine->setThrust(nThrust, kLongBurnMs));
  proceedEnviroment();

  // 3. turn the nose a quarter turn toward +Y while the engine is thrusting
  const uint64_t nTurnUs = 1000000;
  ship.m_pRemote->rotate(kQuarterTurn, nTurnUs);
  justWait(static_cast<uint32_t>(nTurnUs / 1000));
  EXPECT_TRUE(ship.m_pRemote->getOrientation().almostEqual(
      geometry::Vector(0, 1), 1e-6));

  // 4. the reported thrust is still the magnitude that was set
  uint32_t applied = 0;
  ASSERT_TRUE(engine->getThrust(applied));
  EXPECT_EQ(nThrust, applied);

  // 5. further acceleration follows that nose and keeps the same magnitude
  const geometry::Vector expectedAcc =
      ship.m_pRemote->getOrientation().ofLength(nThrust) / nMass;
  const uint64_t t0 = utils::GlobalClock::now();
  const geometry::Vector v0 = ship.m_pRemote->getVelocity();
  justWait(1000);
  const double dt = (utils::GlobalClock::now() - t0) / 1000000.0;
  const geometry::Vector velocity = ship.m_pRemote->getVelocity();
  EXPECT_TRUE(velocity.almostEqual(v0 + expectedAcc * dt, 1e-3));
  EXPECT_NEAR(expectedAcc.getLength(), nThrust / nMass, 1e-9);
}

TEST_F(HoverEngineTests, MonitorSnapshotAndAppliedThrust)
{
  // 1. connect and spawn a ship with a hover engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  HoverEngineBinding engine = Helper::spawnHoverEngine(
      ship, Helper::HoverEngineParams().maxThrust(nMaxThrust));

  // 2. subscribe to thrust monitoring
  client::HoverEnginePtr pMonitor = openHoverEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  uint32_t snapshot = 1;
  ASSERT_TRUE(pMonitor->monitor(snapshot));
  EXPECT_EQ(0u, snapshot);

  // 3. apply a thrust
  const uint32_t nThrust = 100;
  ASSERT_TRUE(engine->setThrust(nThrust, kLongBurnMs));

  // 4. monitor receives the applied magnitude
  uint32_t applied = 0;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(nThrust, applied);

  // 5. change_thrust session has no reply
  spex::IHoverEngine unexpected;
  ASSERT_FALSE(engine->pick(unexpected));
}

TEST_F(HoverEngineTests, MonitorClampedThrust)
{
  // 1. connect and spawn a ship with a hover engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  HoverEngineBinding engine = Helper::spawnHoverEngine(
      ship, Helper::HoverEngineParams().maxThrust(nMaxThrust));

  // 2. subscribe to thrust monitoring
  client::HoverEnginePtr pMonitor = openHoverEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  uint32_t snapshot = 0;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. request a thrust above max_thrust
  ASSERT_TRUE(engine->setThrust(nMaxThrust * 2, kLongBurnMs));

  // 4. monitor receives the clamped magnitude
  uint32_t applied = 0;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(nMaxThrust, applied);
}

TEST_F(HoverEngineTests, MonitorDurationExpiry)
{
  // 1. connect and spawn a ship with a hover engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  HoverEngineBinding engine = Helper::spawnHoverEngine(
      ship, Helper::HoverEngineParams().maxThrust(100000));

  // 2. subscribe to thrust monitoring
  client::HoverEnginePtr pMonitor = openHoverEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pMonitor);

  uint32_t snapshot = 0;
  ASSERT_TRUE(pMonitor->monitor(snapshot));

  // 3. start a short burn
  const uint32_t nThrust = 100;
  ASSERT_TRUE(engine->setThrust(nThrust, 200));

  uint32_t applied = 0;
  ASSERT_TRUE(pMonitor->waitThrust(applied));
  EXPECT_EQ(nThrust, applied);

  // 4. wait for the burn to expire
  uint32_t expired = 1;
  ASSERT_TRUE(pMonitor->waitThrust(expired, 500));
  EXPECT_EQ(0u, expired);
}

TEST_F(HoverEngineTests, SeveralSessionsMayMonitor)
{
  // 1. connect and spawn a ship with a hover engine
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  HoverEngineBinding engine = Helper::spawnHoverEngine(
      ship, Helper::HoverEngineParams().maxThrust(100000));

  // 2. open two monitoring sessions
  client::HoverEnginePtr pFirst = openHoverEngineSession(ship, engine.m_nSlotId);
  client::HoverEnginePtr pSecond = openHoverEngineSession(ship, engine.m_nSlotId);
  ASSERT_TRUE(pFirst);
  ASSERT_TRUE(pSecond);

  uint32_t firstSnapshot = 1;
  uint32_t secondSnapshot = 1;
  ASSERT_TRUE(pFirst->monitor(firstSnapshot));
  ASSERT_TRUE(pSecond->monitor(secondSnapshot));
  EXPECT_EQ(0u, firstSnapshot);
  EXPECT_EQ(0u, secondSnapshot);

  // 3. apply thrust and both sessions receive it
  const uint32_t nThrust = 130;
  ASSERT_TRUE(engine->setThrust(nThrust, kLongBurnMs));

  uint32_t firstApplied = 0;
  uint32_t secondApplied = 0;
  ASSERT_TRUE(pFirst->waitThrust(firstApplied));
  ASSERT_TRUE(pSecond->waitThrust(secondApplied));
  EXPECT_EQ(nThrust, firstApplied);
  EXPECT_EQ(nThrust, secondApplied);

  // 4. closing the first session does not stop the second
  ASSERT_TRUE(pFirst->disconnect());

  const uint32_t nNextThrust = 80;
  ASSERT_TRUE(engine->setThrust(nNextThrust, kLongBurnMs));

  uint32_t secondNext = 0;
  ASSERT_TRUE(pSecond->waitThrust(secondNext));
  EXPECT_EQ(nNextThrust, secondNext);
}

} // namespace autotests

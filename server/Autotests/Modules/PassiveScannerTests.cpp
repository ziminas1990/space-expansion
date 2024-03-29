#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Utils/Clock.h>
#include <Utils/Randomizer.h>
#include <World/Grid.h>
#include <World/Player.h>
#include <Modules/Managers.h>
#include <Modules/PassiveScanner/PassiveScanner.h>
#include <Newton/NewtonEngine.h>
#include <Geometry/Rectangle.h>
#include <Modules/Ship/Ship.h>
#include <World/CelestialBodies/Asteroid.h>
#include <World/CelestialBodies/AsteroidGenerator.h>
#include <Autotests/TestUtils/Connector.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>
#include <Autotests/ClientSDK/Modules/ClientPassiveScanner.h>
#include <Autotests/Modules/ModulesTestFixture.h>
#include <Autotests/Modules/Helper.h>

namespace autotests {

using ObjectData = client::ClientPassiveScanner::ObjectData;

class UpdatesJournal {
  using UpdatesCounter = std::map<uint32_t, uint32_t>;
  // obiectId -> counter

  UpdatesCounter m_asteroids;
  UpdatesCounter m_ships;

  template<typename K, typename V>
  static std::set<uint32_t> keys(const std::map<K, V>& container) {
    std::set<uint32_t> ids;
    for (const auto& item: container) {
      ids.insert(item.first);
    }
    return ids;
  }

public:

  void onUpdate(const ObjectData& data) {
    switch (data.m_eType) {
      case world::ObjectType::eAsteroid: {
        auto itCounter = m_asteroids.find(data.m_nObjectId);
        if (itCounter != m_asteroids.end()) {
          itCounter->second++;
        } else {
          m_asteroids[data.m_nObjectId] = 1;
        }
        return;
      }
      case world::ObjectType::eShip: {
        auto itCounter = m_ships.find(data.m_nObjectId);
        if (itCounter != m_ships.end()) {
          itCounter->second++;
        } else {
          m_ships[data.m_nObjectId] = 1;
        }
        return;
      }
      default:
        assert(false);
        return;
    }
  }

  std::set<uint32_t> allAsteroidsIds() const {
    return keys(m_asteroids);
  }

  std::set<uint32_t> allShipsIds() const {
    return keys(m_ships);
  }

  uint32_t totalUpdatesFor(const world::AsteroidUptr& asteroid) const {
    return m_asteroids.at(asteroid->getAsteroidId());
  }

  bool operator==(const UpdatesJournal& other) const {
    return m_ships == other.m_ships && m_asteroids == other.m_asteroids;
  }
};

class PassiveScannerTests : public ModulesTestFixture
{
public:
  PassiveScannerTests()
    : m_sScannerName("PassiveScanner_1")
    , m_nRadiusKm(1000)         // 1000 km
    , m_nMaxUpdateTimeMs(5000)  // 5 sec
    , m_nPassiveScannerSlot(modules::Commutator::invalidSlot())
  {}

  void SetUp() override;

  virtual std::shared_ptr<world::Grid> createGlobalGrid() override {
    // Each test will create it's own global grid
    return nullptr;
  }

  client::ClientCommutatorPtr shipCommutator();
  client::ClientPassiveScannerPtr spawnScanner();

protected:

  bool collectUpdates(
      UpdatesJournal& journal,
      client::ClientPassiveScannerPtr pScanner,
      uint32_t nCollectingTimeMs);

  void pickAllUpdates(
      UpdatesJournal& journal,
      client::ClientPassiveScannerPtr pScanner);

protected:
  // Scanner params
  std::string m_sScannerName;
  uint32_t    m_nRadiusKm;
  uint32_t    m_nMaxUpdateTimeMs;
  uint32_t    m_nShipSlot;
  uint32_t    m_nPassiveScannerSlot;

  client::RootSessionPtr      m_pRootSession;
  client::ClientCommutatorPtr m_pRootCommutator;
  modules::ShipPtr            m_pShip;
  modules::PassiveScannerPtr  m_pPassiveScanner;
};

void PassiveScannerTests::SetUp()
{
  ModulesTestFixture::SetUp();

  // To speed up tests:
  m_clock.setDebugTickUs(50000);

  // Create a ship with a passive scanner on it
  m_pShip = std::make_shared<modules::Ship>(
        "Scout", "scout-1", m_pPlayer, 1000, 10);
  m_nShipSlot = m_pPlayer->onNewShip(m_pShip);

  m_pPassiveScanner = std::make_shared<modules::PassiveScanner>(
        std::string(m_sScannerName),
        m_pPlayer,
        m_nRadiusKm,
        m_nMaxUpdateTimeMs);
  m_nPassiveScannerSlot = m_pShip->installModule(m_pPassiveScanner);
  ASSERT_NE(modules::Commutator::invalidSlot(), m_nPassiveScannerSlot);

  m_pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(m_pRootSession);
}

client::ClientCommutatorPtr PassiveScannerTests::shipCommutator()
{
  if (!m_pRootCommutator) {
    m_pRootCommutator = Helper::openCommutatorSession(*this, m_pRootSession);
    EXPECT_TRUE(m_pRootCommutator);
    if (!m_pRootCommutator) {
      return nullptr;
    }
  }

  client::Router::SessionPtr pTunnel =
      m_pRootCommutator->openSession(m_nShipSlot);
  if (!pTunnel) {
    return client::ClientCommutatorPtr();
  }

  client::ClientCommutatorPtr pCommutator =
      std::make_shared<client::ClientCommutator>(m_pRouter);
  pCommutator->attachToChannel(pTunnel);
  return pCommutator;
}

client::ClientPassiveScannerPtr
PassiveScannerTests::spawnScanner()
{
  client::ClientCommutatorPtr pCommutator = shipCommutator();

  client::Router::SessionPtr pTunnel =
      pCommutator->openSession(m_nPassiveScannerSlot);
  if (!pTunnel) {
    return client::ClientPassiveScannerPtr();
  }

  client::ClientPassiveScannerPtr pScanner =
      std::make_shared<client::ClientPassiveScanner>();
  pScanner->attachToChannel(pTunnel);
  return pScanner;
}

bool
PassiveScannerTests::collectUpdates(
    UpdatesJournal& journal,
    client::ClientPassiveScannerPtr pScanner,
    uint32_t nCollectingTimeMs)
{
  const uint64_t nStopAt = m_clock.now() + nCollectingTimeMs * 1000;
  while (m_clock.now() < nStopAt) {
    std::vector<ObjectData> update;
    if (!pScanner->waitUpdate(update)) {
      return false;
    }

    for (const ObjectData& data: update) {
      journal.onUpdate(data);
    }
  }
  return true;
}

void PassiveScannerTests::pickAllUpdates(
    UpdatesJournal& journal,
    client::ClientPassiveScannerPtr pScanner)
{
  std::vector<ObjectData> update;
  while(pScanner->pickUpdate(update)) {
    for (const ObjectData& data: update) {
      journal.onUpdate(data);
    }
  }
}

struct Tool {
  // Number of helpers to authoring tests

  static void moveShipToRandomPosition(modules::ShipPtr pShip,
                                       const geometry::Rectangle& area)
  {
    geometry::Point position;
    utils::Randomizer::yield(position, area);
    pShip->moveTo(position);
  }

  static world::AsteroidUptr spawnAsteroid()
  {
    return std::make_unique<world::Asteroid>(
          utils::Randomizer::yield<double>(1, 100),
          world::ResourcesArray().metals(1).silicates(1).ice(1).stones(10),
          utils::Randomizer::yield<uint32_t>(1, 10000));
  }

  static world::AsteroidUptr spawnAsteroid(const geometry::Rectangle& area)
  {
    world::Asteroid::Uptr pAsteroid = spawnAsteroid();
    geometry::Point position;
    utils::Randomizer::yield(position, area);
    pAsteroid->moveTo(position);
    return pAsteroid;
  }

  static world::AsteroidUptr spawnAsteroid(const geometry::Point& center,
                                           double radius)
  {
    world::Asteroid::Uptr pAsteroid = spawnAsteroid();

    geometry::Point position;
    utils::Randomizer::yield(position, center, radius);
    pAsteroid->moveTo(position);
    return pAsteroid;
  }

  static modules::ShipPtr spawnShip(const geometry::Rectangle& area,
                                    world::PlayerWeakPtr pOwner)
  {
    modules::ShipPtr pShip = std::make_unique<modules::Ship>(
          "SomeShipType", "SomeShip", pOwner, 1000000, 10);

    geometry::Point position;
    utils::Randomizer::yield(position, area);
    pShip->moveTo(position);
    return pShip;
  }

  static bool inRange(const modules::ShipPtr& pShip,
                      const newton::PhysicalObject* pObject,
                      uint32_t nRadiusKm)
  {
    double distance = pShip->getPosition().distance(pObject->getPosition());
    return distance < nRadiusKm * 1000;
  }
};

TEST_F(PassiveScannerTests, getSpecification)
{
  const uint32_t nScanningRadius = m_nRadiusKm * 1000;
  // Scanning radius covers 5 cells, Grid has 25 * 25 size
  world::Grid grid(25, nScanningRadius / 5);
  world::Grid::setGlobal(&grid);

  client::ClientPassiveScannerPtr pScanner = spawnScanner();
  ASSERT_TRUE(pScanner);

  ASSERT_TRUE(pScanner->sendSpecificationReq());

  client::ClientPassiveScanner::Specification spec;
  ASSERT_TRUE(pScanner->waitSpecification(spec));

  EXPECT_EQ(m_nRadiusKm, spec.m_nScanningRadiusKm);
  EXPECT_EQ(m_nMaxUpdateTimeMs, spec.m_nMaxUpdateTimeMs);
}

TEST_F(PassiveScannerTests, ScanMultipleObjects)
{
  utils::Randomizer::setPattern(123);

  const uint32_t nScanningRadius = m_nRadiusKm * 1000;
  // Scanning radius covers 5 cells, Grid has 25 * 25 size
  world::Grid grid(25, nScanningRadius / 5);
  world::Grid::setGlobal(&grid);

  client::ClientPassiveScannerPtr pScanner = spawnScanner();
  ASSERT_TRUE(pScanner);

  // Let's create a number of random positioned asteroids and ships
  std::vector<world::Asteroid::Uptr> asteroids;
  for (size_t i = 0; i < 500; ++i) {
    asteroids.push_back(Tool::spawnAsteroid(grid.asRect()));
  }

  world::PlayerPtr pOtherPlayer = world::Player::makeDummy("OtherPlayer");
  std::vector<modules::ShipPtr> ships;
  for (size_t i = 0; i < 200; ++i) {
    ships.push_back(Tool::spawnShip(grid.asRect(), pOtherPlayer));
  }

  for (uint32_t i = 0; i < 50; ++i) {
    // Move ship to random position
    utils::Randomizer::setPattern(i);
    Tool::moveShipToRandomPosition(m_pShip, grid.asRect());
    // Since ships has been moved, we should reset scanner
    m_pPassiveScanner->reset();
    ASSERT_TRUE(pScanner->sendMonitor());
    ASSERT_TRUE(pScanner->waitMonitorAck());

    UpdatesJournal journal;
    ASSERT_TRUE(collectUpdates(journal, pScanner, m_nMaxUpdateTimeMs));

    std::set<uint32_t> reportedAsteroids = journal.allAsteroidsIds();

    // Figure out which objects should be reported by scanner
    std::set<uint32_t> expectedAsteroids;
    for (const world::Asteroid::Uptr& pAsteroid: asteroids) {
      if (Tool::inRange(m_pShip, pAsteroid.get(), m_nRadiusKm)) {
        expectedAsteroids.insert(pAsteroid->getAsteroidId());
      }
    }
    ASSERT_EQ(expectedAsteroids, journal.allAsteroidsIds());

    std::set<uint32_t> expectedShips;
    for (const modules::ShipPtr& pShip: ships) {
      if (Tool::inRange(m_pShip, pShip.get(), m_nRadiusKm)) {
        expectedShips.insert(pShip->getShipId());
      }
    }
    ASSERT_EQ(expectedShips, journal.allShipsIds());
  }
}

TEST_F(PassiveScannerTests, ScanLotsOfObjectsNearby)
{
  // This test check that even if module's bandwidth is limited (it can report
  // about ~320 objects per second), all objects will be reported sooner or
  // later

  utils::Randomizer::setPattern(567);

  const uint32_t nScanningRadius = m_nRadiusKm * 1000;
  // Scanning radius covers 1 cells, Grid has 1 * 1 size
  world::Grid grid(1, 2 * nScanningRadius);
  world::Grid::setGlobal(&grid);

  // Let's create a huge amount of random positioned asteroids
  // But all asteroids are in range, so they should be reported
  std::vector<world::Asteroid::Uptr> asteroids;
  for (size_t i = 0; i < 3000; ++i) {
    asteroids.push_back(
          Tool::spawnAsteroid(
            m_pShip->getPosition(), nScanningRadius * 0.99));
  }

  client::ClientPassiveScannerPtr pScanner = spawnScanner();
  ASSERT_TRUE(pScanner);
  ASSERT_TRUE(pScanner->sendMonitor());
  ASSERT_TRUE(pScanner->waitMonitorAck());

  UpdatesJournal journal;
  ASSERT_TRUE(collectUpdates(journal, pScanner, 5 * m_nMaxUpdateTimeMs));

  std::set<uint32_t> reportedAsteroids = journal.allAsteroidsIds();

  // Figure out which objects should be reported by scanner
  std::set<uint32_t> expectedAsteroids;
  for (const world::Asteroid::Uptr& pAsteroid: asteroids) {
    expectedAsteroids.insert(pAsteroid->getAsteroidId());
  }
  ASSERT_EQ(expectedAsteroids, journal.allAsteroidsIds());
}

TEST_F(PassiveScannerTests, SmoothUpdates)
{
  // Let say we have maxScanningTime=10 and there is an object on the edge of
  // scanning radius, that should be updated once in 9 seconds.
  // It means, that passive scanner performs global scan (with updating
  // of internal cache) one in 10 seconds, but should send update for an object
  // every 9 seconds. So, after 91 seconds we should get 10 updates.

  utils::Randomizer::setPattern(382);

  const uint32_t nScanningRadius = m_nRadiusKm * 1000;
  // Scanning radius covers 1 cells, Grid has 1 * 1 size
  world::Grid grid(1, 2 * nScanningRadius);
  world::Grid::setGlobal(&grid);

  // Spawn an object on the edge of the scanning radius
  world::Asteroid::Uptr pAsteroid = Tool::spawnAsteroid();
  {
    // Move asteroid to the edge of scanning radius
    const double r = nScanningRadius * 0.9;
    geometry::Point position;
    utils::Randomizer::yield(position, m_pShip->getPosition(), r, r);
    pAsteroid->moveTo(position);
  }

  client::ClientPassiveScannerPtr pScanner = spawnScanner();
  ASSERT_TRUE(pScanner);
  ASSERT_TRUE(pScanner->sendMonitor());
  ASSERT_TRUE(pScanner->waitMonitorAck());

  const uint32_t nTotalCycles = 500;
  UpdatesJournal journal;
  ASSERT_TRUE(collectUpdates(
                journal, pScanner, nTotalCycles * m_nMaxUpdateTimeMs));
  ASSERT_GT(journal.totalUpdatesFor(pAsteroid), nTotalCycles);
}

TEST_F(PassiveScannerTests, MultipleSessions)
{
  utils::Randomizer::setPattern(473);

  const uint32_t nScanningRadius = m_nRadiusKm * 1000;
  // Scanning radius covers 1 cells, Grid has 1 * 1 size
  world::Grid grid(1, 2 * nScanningRadius);
  world::Grid::setGlobal(&grid);

  std::vector<world::Asteroid::Uptr> asteroids;
  for (size_t i = 0; i < 100; ++i) {
    asteroids.push_back(
          Tool::spawnAsteroid(
            m_pShip->getPosition(), nScanningRadius * 0.99));
  }

  struct SessionAndJournal {
    client::ClientPassiveScannerPtr m_pScanner;
    UpdatesJournal                  m_journal;
  };

  const size_t totalSessions = 8;
  std::array<SessionAndJournal, totalSessions> sessions;

  for (size_t i = 0; i < totalSessions; ++i) {
    client::ClientPassiveScannerPtr pScanner = spawnScanner();
    ASSERT_TRUE(pScanner);
    sessions[i] = SessionAndJournal{pScanner, UpdatesJournal()};
  }

  // Send start monitoring to each of them
  for (SessionAndJournal& session: sessions) {
    ASSERT_TRUE(session.m_pScanner->sendMonitor());
  }
  // Await for ack
  for (SessionAndJournal& session: sessions) {
    ASSERT_TRUE(session.m_pScanner->waitMonitorAck());
  }

  justWait(m_nMaxUpdateTimeMs * 10);

  // Pick all updates
  for (SessionAndJournal& session: sessions) {
    pickAllUpdates(session.m_journal, session.m_pScanner);
  }

  // All journals are expected to be equal
  const UpdatesJournal& referenceJournal = sessions[0].m_journal;
  for (SessionAndJournal& session: sessions) {
    ASSERT_EQ(referenceJournal, session.m_journal);
  }
}

}  // namespace autotests

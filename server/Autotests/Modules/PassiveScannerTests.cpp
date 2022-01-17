#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Utils/Clock.h>
#include <Utils/Randomizer.h>
#include <World/Grid.h>
#include <World/Player.h>
#include <Modules/PassiveScanner/PassiveScanner.h>
#include <Modules/PassiveScanner/PassiveScannerManager.h>
#include <Modules/Commutator/CommutatorManager.h>
#include <Newton/NewtonEngine.h>
#include <Geometry/Rectangle.h>
#include <Ships/Ship.h>
#include <World/CelestialBodies/Asteroid.h>
#include <World/CelestialBodies/AsteroidGenerator.h>
#include <Autotests/TestUtils/Connector.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>
#include <Autotests/ClientSDK/Modules/ClientPassiveScanner.h>

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
};

class PassiveScannerTests : public ::testing::Test
{
public:
  PassiveScannerTests()
    : m_sScannerName("PassiveScanner_1")
    , m_nRadiusKm(1000)         // 1000 km
    , m_nMaxUpdateTimeMs(5000)  // 5 sec
    , m_nPassiveScannerSlot(modules::Commutator::invalidSlot())
    , m_conveyor(1)
  {}

  void SetUp() override;
  void TearDown() override;

  client::ClientPassiveScannerPtr spawnScanner();

private:
  void proceedEnviroment() {
    m_conveyor.proceed(m_clock.getNextInterval());
  }

protected:

  bool collectUpdates(
      UpdatesJournal& journal,
      client::ClientPassiveScannerPtr pScanner,
      uint32_t nCollectingTimeMs);

  void justWait(uint32_t nTimeMs) {
    const uint64_t nStopAtUs = m_clock.now() + nTimeMs * 1000;
    while (m_clock.now() < nStopAtUs) {
      proceedEnviroment();
    }
  }

protected:
  // Scanner params
  std::string m_sScannerName;
  uint32_t    m_nRadiusKm;
  uint32_t    m_nMaxUpdateTimeMs;
  uint32_t    m_nPassiveScannerSlot;

  // Components on server side
  utils::Clock                      m_clock;
  conveyor::Conveyor                m_conveyor;
  modules::PassiveScannerManagerPtr m_pPassiveScannerManager;
  modules::CommutatorManagerPtr     m_pCommutatorManager;
  newton::NewtonEnginePtr           m_pNewtonEngine;
  std::function<void()>             m_fConveyorProceeder;

  world::PlayerPtr                  m_pPlayer;
  ships::ShipPtr                    m_pShip;
  modules::PassiveScannerPtr        m_pPassiveScanner;

  // Component, that connects client and server sides
  PlayerConnectorPtr   m_pConnection;
  PlayerConnectorGuard m_connectionGuard;

  // Components on client's side
  client::PlayerPipePtr       m_pRootPipe;
  client::ClientCommutatorPtr m_pClientCommutator;

};

void PassiveScannerTests::SetUp()
{
  m_clock.switchToDebugMode();
  m_clock.setDebugTickUs(25000);  // 25ms per tick
  m_clock.proceedRequest(0xFFFFFFFF);  // As long as it is required
  m_clock.start(true);
  utils::GlobalClock::set(&m_clock);

  m_pCommutatorManager = std::make_shared<modules::CommutatorManager>();
  m_pPassiveScannerManager = std::make_shared<modules::PassiveScannerManager>();
  m_pNewtonEngine = std::make_shared<newton::NewtonEngine>();
  m_conveyor.addLogicToChain(m_pNewtonEngine);
  m_conveyor.addLogicToChain(m_pCommutatorManager);
  m_conveyor.addLogicToChain(m_pPassiveScannerManager);

  m_fConveyorProceeder = [this]() { this->proceedEnviroment(); };

  // Components on server
  m_pPlayer = world::Player::makeDummy("Player-1");
  m_pPassiveScanner = std::make_shared<modules::PassiveScanner>(
        std::string(m_sScannerName),
        m_pPlayer,
        m_nRadiusKm,
        m_nMaxUpdateTimeMs);
  m_pShip = std::make_shared<ships::Ship>(
        "Scout", "scout-1", m_pPlayer, 1000, 10);
  m_nPassiveScannerSlot = m_pShip->installModule(m_pPassiveScanner);
  ASSERT_NE(modules::Commutator::invalidSlot(), m_nPassiveScannerSlot);

  m_pConnection = std::make_shared<PlayerConnector>(1);

  // Components on client
  m_pRootPipe = std::make_shared<client::PlayerPipe>();
  m_pRootPipe->setProceeder(m_fConveyorProceeder);

  m_pClientCommutator = std::make_shared<client::ClientCommutator>();
  m_pClientCommutator->attachToChannel(m_pRootPipe);

  m_connectionGuard.link(m_pConnection, m_pShip->getCommutator(), m_pRootPipe);
}

void PassiveScannerTests::TearDown()
{
  m_pClientCommutator->detachChannel();
  utils::GlobalClock::reset();
  // Destructor will destroy 'm_connectionGuard' object, that will unlink
  // the rest of the components
}

client::ClientPassiveScannerPtr
PassiveScannerTests::spawnScanner()
{
  client::TunnelPtr pTunnel =
      m_pClientCommutator->openTunnel(m_nPassiveScannerSlot);
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

struct Tool {
  // Number of helpers to authoring tests

  static void moveShipToRandomPosition(ships::ShipPtr pShip,
                                       const geometry::Rectangle& area)
  {
    geometry::Point position;
    utils::Randomizer::yield(position, area);
    pShip->moveTo(position);
  }

  static world::AsteroidUptr spawnAsteroid(const geometry::Rectangle& area)
  {
    world::Asteroid::Uptr pAsteroid = std::make_unique<world::Asteroid>(
          utils::Randomizer::yield<double>(1, 100),
          utils::Randomizer::yield<double>(10000, 1000000),
          world::AsteroidComposition(1, 1, 1, 10),
          utils::Randomizer::yield<uint32_t>(1, 10000));

    geometry::Point position;
    utils::Randomizer::yield(position, area);
    pAsteroid->moveTo(position);
    return pAsteroid;
  }

  static ships::ShipPtr spawnShip(const geometry::Rectangle& area,
                                  world::PlayerWeakPtr pOwner)
  {
    ships::ShipPtr pShip = std::make_unique<ships::Ship>(
          "SomeShipType", "SomeShip", pOwner, 1000000, 10);

    geometry::Point position;
    utils::Randomizer::yield(position, area);
    pShip->moveTo(position);
    return pShip;
  }

  static bool inRange(const ships::ShipPtr& pShip,
                      const newton::PhysicalObject* pObject,
                      uint32_t nRadiusKm)
  {
    double distance = pShip->getPosition().distance(pObject->getPosition());
    return distance < nRadiusKm * 1000;
  }
};

TEST_F(PassiveScannerTests, getSpecification) {

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
  std::vector<ships::ShipPtr> ships;
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
    ASSERT_TRUE(collectUpdates(journal, pScanner, 2 * m_nMaxUpdateTimeMs));

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
    for (const ships::ShipPtr& pShip: ships) {
      if (Tool::inRange(m_pShip, pShip.get(), m_nRadiusKm)) {
        expectedShips.insert(pShip->getShipId());
      }
    }
    ASSERT_EQ(expectedShips, journal.allShipsIds());
  }
}

}  // namespace autotests

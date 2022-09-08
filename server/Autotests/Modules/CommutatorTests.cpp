#include <Autotests/Modules/ModulesTestFixture.h>

#include <Modules/Engine/Engine.h>
#include <Autotests/Modules/Helper.h>

namespace autotests {

class CommutatorTests : public ModulesTestFixture
{};

TEST_F(CommutatorTests, Breath)
{
  // Check that commutator works and we can spawn ships and modules and
  // attach to them

  Connection connection = Helper::connect(*this, 5);

  ShipBinding ship = Helper::spawnShip(
    *this, connection, m_pPlayer, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));

  client::EngineSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));
  ASSERT_EQ(nMaxThrust, spec.nMaxThrust);
}

TEST_F(CommutatorTests, Monitoring)
{
  Connection connection = Helper::connect(*this, 3);

  std::vector<Connection> monitoringSessions;
  for (uint32_t nConnectionId = 4; nConnectionId <= 7; ++nConnectionId) {
    Connection monitoring = Helper::connect(*this, nConnectionId);
    ASSERT_TRUE(monitoring->monitoring());
    monitoringSessions.push_back(std::move(monitoring));
  }

  ShipBinding ship = Helper::spawnShip(
    *this, connection, m_pPlayer, geometry::Point(0, 0), Helper::ShipParams());

  for (Connection& monitoring: monitoringSessions) {
    spex::ICommutator::ModuleInfo info;
    ASSERT_TRUE(monitoring->waitModuleAttached(info));
    EXPECT_EQ(info.slot_id(), ship.m_nSlotId);
  }

  // Detach module
  ship.m_pRemote->onDoestroyed();

  for (Connection& monitoring: monitoringSessions) {
    uint32_t nSlotId;
    ASSERT_TRUE(monitoring->waitModuleDetached(nSlotId));
    EXPECT_EQ(nSlotId, ship.m_nSlotId);
  }
}

TEST_F(CommutatorTests, CloseSession)
{
  // Check that if root session is closed, all other sessions will be closed
  // as well.

  Connection connection = Helper::connect(*this, 5);

  ShipBinding ship = Helper::spawnShip(
    *this, connection, m_pPlayer, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));

  // If session to the ship is closed, engine should be avaliable anyway
  ASSERT_TRUE(ship->disconnect());

  client::EngineSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));

  // If a root session is closed, engine session should also be closed
  ASSERT_TRUE(connection->disconnect());
  ASSERT_TRUE(engine->waitCloseInd());
}

TEST_F(CommutatorTests, CloseAdditionalSession)
{
  const uint32_t nConnectionId = 5;
  Connection connection = Helper::connect(*this, nConnectionId);

  std::optional<Connection> additionalSession = Helper::openAdditionalSession(*this, nConnectionId);
  ASSERT_TRUE(additionalSession.has_value());

  ShipBinding ship = Helper::spawnShip(
    *this, *additionalSession, m_pPlayer, geometry::Point(0, 0), Helper::ShipParams());

  // Check that 'ship' is alive
  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));
  client::EngineSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));

  // If additional session is closed, engine session should NOT be closed
  ASSERT_TRUE((*additionalSession)->disconnect());
  ASSERT_FALSE(engine->waitCloseInd(100));

  // If a root session is closed, other sessions should also be closed
  ASSERT_TRUE(connection->disconnect());
  EXPECT_TRUE(ship->waitCloseInd());
  EXPECT_TRUE(engine->waitCloseInd());
}

}  // namespace autotests
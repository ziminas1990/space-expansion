#include "Autotests/ClientSDK/Modules/ClientCommutator.h"
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

  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, m_pPlayer, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));

  client::EngineSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));
  ASSERT_EQ(nMaxThrust, spec.nMaxThrust);
}

TEST_F(CommutatorTests, Monitoring)
{
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  std::vector<client::ClientCommutatorPtr> monitoringSessions;
  for (uint32_t nConnectionId = 4; nConnectionId <= 7; ++nConnectionId) {
    client::ClientCommutatorPtr pSession =
      Helper::openCommutatorSession(*this, pRootSession);
    ASSERT_TRUE(pSession);
    ASSERT_TRUE(pSession->monitoring());
    monitoringSessions.push_back(std::move(pSession));
  }

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, m_pPlayer, geometry::Point(0, 0), Helper::ShipParams());

  for (client::ClientCommutatorPtr& pSession: monitoringSessions) {
    spex::ICommutator::ModuleInfo info;
    ASSERT_TRUE(pSession->waitModuleAttached(info));
    EXPECT_EQ(info.slot_id(), ship.m_nSlotId);
  }

  // Detach module
  ship.m_pRemote->onDoestroyed();

  for (client::ClientCommutatorPtr& pSession: monitoringSessions) {
    uint32_t nSlotId;
    ASSERT_TRUE(pSession->waitModuleDetached(nSlotId));
    EXPECT_EQ(nSlotId, ship.m_nSlotId);
  }
}

TEST_F(CommutatorTests, CloseSession)
{
  // Check that if root session is closed, all other sessions will be closed
  // as well.

  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, m_pPlayer, geometry::Point(0, 0), Helper::ShipParams());

  const uint32_t nMaxThrust = 100000;
  EngineBinding engine = Helper::spawnEngine(
    ship, Helper::EngineParams().maxThrust(nMaxThrust));

  // If session to the ship is closed, engine should be avaliable anyway
  ASSERT_TRUE(ship->disconnect());

  client::EngineSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));

  // If a root session is closed, engine session should also be closed
  ASSERT_TRUE(pRootSession->close());
  ASSERT_TRUE(engine->waitCloseInd());
}

}  // namespace autotests
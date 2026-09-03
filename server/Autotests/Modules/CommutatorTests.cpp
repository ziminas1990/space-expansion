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
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

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
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

  for (client::ClientCommutatorPtr& pSession: monitoringSessions) {
    spex::ICommutator::ModuleInfo info;
    ASSERT_TRUE(pSession->waitModuleAttached(info));
    EXPECT_EQ(info.slot_id(), ship.m_nSlotId);
    EXPECT_EQ("Ship", info.module_type());
    EXPECT_EQ("SomeShip", info.module_name());
    EXPECT_EQ("Ship/SomeType", info.blueprint_name());
  }

  // Detach module
  ship.m_pRemote->onDoestroyed();

  for (client::ClientCommutatorPtr& pSession: monitoringSessions) {
    uint32_t nSlotId;
    ASSERT_TRUE(pSession->waitModuleDetached(nSlotId));
    EXPECT_EQ(nSlotId, ship.m_nSlotId);
  }
}

TEST_F(CommutatorTests, ShipModuleInfoReportsFixedTypeAndBlueprint)
{
  // 1. connect and open a commutator session
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  // 2. spawn a ship without opening a tunnel to it
  ShipBinding ship = Helper::spawnShip(
    *this, pCommutator, geometry::Point(0, 0),
    Helper::ShipParams().shipType("Civilian-Miner").shipName("Miner-1"));

  // 3. list attached modules and check type, name, and blueprint
  client::ModulesList attached;
  ASSERT_TRUE(pCommutator->getAttachedModulesList(attached));

  const client::ModuleInfo* pShipInfo = nullptr;
  for (const client::ModuleInfo& info : attached) {
    if (info.nSlotId == ship.m_nSlotId) {
      pShipInfo = &info;
      break;
    }
  }
  ASSERT_TRUE(pShipInfo);
  EXPECT_EQ("Ship", pShipInfo->sModuleType);
  EXPECT_EQ("Miner-1", pShipInfo->sModuleName);
  EXPECT_EQ("Ship/Civilian-Miner", pShipInfo->sBlueprintName);
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
    *this, pCommutator, geometry::Point(0, 0), Helper::ShipParams());

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
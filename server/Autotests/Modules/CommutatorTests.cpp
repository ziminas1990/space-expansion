#include "Autotests/ClientSDK/Modules/ClientCommutator.h"
#include <Autotests/Modules/ModulesTestFixture.h>

#include <Modules/RCS/RCS.h>
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
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  client::RCSSpecification spec;
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

TEST_F(CommutatorTests, AllModulesInfoOnEmptyCommutator)
{
  // 1. connect and open a commutator session
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);

  // 2. request every module
  client::ModulesList attached;
  ASSERT_TRUE(pCommutator->getAttachedModulesList(attached));

  // 3. the reply is one message with an empty list
  EXPECT_TRUE(attached.empty());

  // 4. the same session answers the next command
  uint32_t nTotalSlots = 0;
  ASSERT_TRUE(pCommutator->getTotalSlots(nTotalSlots));
  EXPECT_EQ(0u, nTotalSlots);
}

TEST_F(CommutatorTests, AllModulesInfoListsInstalledModulesOnly)
{
  // 1. connect and open a command session and a monitoring session
  client::RootSessionPtr pRootSession = Helper::connect(*this, 5);
  ASSERT_TRUE(pRootSession);
  client::ClientCommutatorPtr pCommutator =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pCommutator);
  client::ClientCommutatorPtr pMonitor =
      Helper::openCommutatorSession(*this, pRootSession);
  ASSERT_TRUE(pMonitor);
  ASSERT_TRUE(pMonitor->monitoring());

  // 2. spawn two ships
  ShipBinding first = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0),
      Helper::ShipParams().shipName("Miner-1"));
  ShipBinding second = Helper::spawnShip(
      *this, pCommutator, geometry::Point(0, 0),
      Helper::ShipParams().shipName("Miner-2"));

  spex::ICommutator::ModuleInfo attached;
  ASSERT_TRUE(pMonitor->waitModuleAttached(attached));
  ASSERT_TRUE(pMonitor->waitModuleAttached(attached));

  // 3. one reply lists both ships
  client::ModulesList modules;
  ASSERT_TRUE(pCommutator->getAttachedModulesList(modules));
  ASSERT_EQ(2u, modules.size());
  EXPECT_EQ(first.m_nSlotId, modules[0].nSlotId);
  EXPECT_EQ("Miner-1", modules[0].sModuleName);
  EXPECT_EQ("Ship", modules[0].sModuleType);
  EXPECT_EQ(second.m_nSlotId, modules[1].nSlotId);
  EXPECT_EQ("Miner-2", modules[1].sModuleName);

  // 4. the same session answers the next command
  uint32_t nTotalSlots = 0;
  ASSERT_TRUE(pCommutator->getTotalSlots(nTotalSlots));
  EXPECT_EQ(2u, nTotalSlots);

  // 5. destroy the first ship and wait until its slot is empty
  first.m_pRemote->onDoestroyed();
  uint32_t nDetachedSlot = 0;
  ASSERT_TRUE(pMonitor->waitModuleDetached(nDetachedSlot));
  EXPECT_EQ(first.m_nSlotId, nDetachedSlot);

  // 6. the list keeps the remaining ship and skips the empty slot
  modules.clear();
  ASSERT_TRUE(pCommutator->getAttachedModulesList(modules));
  ASSERT_EQ(1u, modules.size());
  EXPECT_EQ(second.m_nSlotId, modules[0].nSlotId);
  EXPECT_EQ("Miner-2", modules[0].sModuleName);

  ASSERT_TRUE(pCommutator->getTotalSlots(nTotalSlots));
  EXPECT_EQ(2u, nTotalSlots);

  // 7. a request for one slot still returns a single module description
  client::ModuleInfo emptySlot;
  ASSERT_TRUE(pCommutator->getModuleInfo(first.m_nSlotId, emptySlot));
  EXPECT_EQ(first.m_nSlotId, emptySlot.nSlotId);
  EXPECT_EQ("empty", emptySlot.sModuleType);
  EXPECT_TRUE(emptySlot.sModuleName.empty());
  EXPECT_TRUE(emptySlot.sBlueprintName.empty());

  client::ModuleInfo occupiedSlot;
  ASSERT_TRUE(pCommutator->getModuleInfo(second.m_nSlotId, occupiedSlot));
  EXPECT_EQ(second.m_nSlotId, occupiedSlot.nSlotId);
  EXPECT_EQ("Ship", occupiedSlot.sModuleType);
  EXPECT_EQ("Miner-2", occupiedSlot.sModuleName);
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
  RCSBinding engine = Helper::spawnRCS(
    ship, Helper::RCSParams().maxThrust(nMaxThrust));

  // If session to the ship is closed, engine should be avaliable anyway
  ASSERT_TRUE(ship->disconnect());

  client::RCSSpecification spec;
  ASSERT_TRUE(engine->getSpecification(spec));

  // If a root session is closed, engine session should also be closed
  ASSERT_TRUE(pRootSession->close());
  ASSERT_TRUE(engine->waitCloseInd());
}

}  // namespace autotests
#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Utils/Clock.h>
#include <Modules/PassiveScanner/PassiveScanner.h>
#include <Modules/PassiveScanner/PassiveScannerManager.h>
#include <Modules/Commutator/CommutatorManager.h>
#include <Ships/Ship.h>
#include <Autotests/TestUtils/Connector.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>
#include <Autotests/ClientSDK/Modules/ClientPassiveScanner.h>

namespace autotests {

class PassiveScannerTests : public ::testing::Test
{
public:
  PassiveScannerTests()
    : m_sScannerName("PassiveScanner_1")
    , m_nRadiusKm(1000)        // 1000 km
    , m_nMaxUpdateTimeMs(5000)  // 5 sec
    , m_nPassiveScannerSlot(modules::Commutator::invalidSlot())
    , m_Conveyor(1)
  {}

  void SetUp() override;
  void TearDown() override;

  client::ClientPassiveScannerPtr spawnScanner();

private:
  void proceedEnviroment() {
    m_Conveyor.proceed(m_clock.getNextInterval());
  }

protected:
  // Scanner params
  std::string m_sScannerName;
  uint32_t    m_nRadiusKm;
  uint32_t    m_nMaxUpdateTimeMs;
  uint32_t    m_nPassiveScannerSlot;

  // Components on server side
  utils::Clock                      m_clock;
  conveyor::Conveyor                m_Conveyor;
  modules::PassiveScannerManagerPtr m_pPassiveScannerManager;
  modules::CommutatorManagerPtr     m_pCommutatorManager;
  std::function<void()>             m_fConveyorProceeder;

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
  m_clock.setDebugTickUs(1000);  // 1ms per tick
  m_clock.proceedRequest(0xFFFFFFFF);  // As long as it is required
  utils::GlobalClock::set(&m_clock);

  m_pCommutatorManager = std::make_shared<modules::CommutatorManager>();
  m_pPassiveScannerManager = std::make_shared<modules::PassiveScannerManager>();
  m_Conveyor.addLogicToChain(m_pCommutatorManager);
  m_Conveyor.addLogicToChain(m_pPassiveScannerManager);

  m_fConveyorProceeder = [this]() { this->proceedEnviroment(); };

  // Components on server
  m_pPassiveScanner = std::make_shared<modules::PassiveScanner>(
        std::string(m_sScannerName),
        world::PlayerWeakPtr(),
        m_nRadiusKm,
        m_nMaxUpdateTimeMs);
  m_pShip = std::make_shared<ships::Ship>(
        "Scout", "scout-1", world::PlayerWeakPtr(), 1000, 10);
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

TEST_F(PassiveScannerTests, getSpecification) {
  client::ClientPassiveScannerPtr pScanner = spawnScanner();
  ASSERT_TRUE(pScanner);

  ASSERT_TRUE(pScanner->sendSpecificationReq());

  client::ClientPassiveScanner::Specification spec;
  ASSERT_TRUE(pScanner->waitSpecification(spec));

  EXPECT_EQ(m_nRadiusKm, spec.m_nScanningRadiusKm);
  EXPECT_EQ(m_nMaxUpdateTimeMs, spec.m_nMaxUpdateTimeMs);
}

}  // namespace autotests

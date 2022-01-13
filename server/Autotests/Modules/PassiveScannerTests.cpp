#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Modules/PassiveScanner/PassiveScanner.h>
#include <Modules/PassiveScanner/PassiveScannerManager.h>
#include <Autotests/TestUtils/BidirectionalChannel.h>
#include <Autotests/TestUtils/SessionsMux.h>
#include <Autotests/ClientSDK/Modules/ClientPassiveScanner.h>

namespace autotests {

class PassiveScannerTests : public ::testing::Test
{
public:
  PassiveScannerTests()
    : m_nRadiusKm(1000)        // 1000 km
    , m_nMaxUpdateTimeMs(5000)  // 5 sec
    , m_Conveyor(1)
    , m_pPassiveScannerManager(
        std::make_shared<modules::PassiveScannerManager>())
    , m_pPassiveScanner(
        std::make_shared<modules::PassiveScanner>(
          "PassiveScanner_1",
          world::PlayerWeakPtr(),
          m_nRadiusKm,
          m_nMaxUpdateTimeMs))
    , m_pChannel(std::make_shared<BidirectionalChannel>())
    , m_pSessionMux(std::make_shared<SessionMux>())
  {}

  void SetUp() override;
  void TearDown() override;

  client::ClientPassiveScannerPtr spawnScanner(uint32_t nSessionId);

private:
  void proceedEnviroment() {
    m_Conveyor.proceed(1000);  // 1ms
  }

protected:
  // Scanner params
  uint32_t m_nRadiusKm;
  uint32_t m_nMaxUpdateTimeMs;

  // Components on server side
  conveyor::Conveyor                m_Conveyor;
  std::function<void()>             m_fConveyorProceeder;
  modules::PassiveScannerManagerPtr m_pPassiveScannerManager;
  modules::PassiveScannerPtr        m_pPassiveScanner;

  // Component, that connects client and server sides
  BidirectionalChannelPtr m_pChannel;

  // Components on client's side
  SessionMuxPtr m_pSessionMux;

};

void PassiveScannerTests::SetUp()
{
  m_Conveyor.addLogicToChain(m_pPassiveScannerManager);

  m_fConveyorProceeder = [this]() { this->proceedEnviroment(); };
  m_pChannel->link(m_pSessionMux, m_pPassiveScanner);
}

void PassiveScannerTests::TearDown()
{
  m_pSessionMux->closeAllSession();
  m_pChannel->unlink();
  m_pPassiveScanner->detachFromChannel();
}

client::ClientPassiveScannerPtr
PassiveScannerTests::spawnScanner(uint32_t nSessionId)
{
  client::PlayerPipePtr pPipe = std::make_shared<client::PlayerPipe>();
  pPipe->setProceeder(m_fConveyorProceeder);

  client::ClientPassiveScannerPtr pScanner =
      std::make_shared<client::ClientPassiveScanner>();
  pScanner->attachToChannel(pPipe);
  m_pSessionMux->openSession(nSessionId, pPipe);
  return pScanner;
}

TEST_F(PassiveScannerTests, getSpecification) {
  const uint32_t nSessionId = 1;
  client::ClientPassiveScannerPtr pScanner = spawnScanner(nSessionId);

  ASSERT_TRUE(pScanner->sendSpecificationReq());

  client::ClientPassiveScanner::Specification spec;
  ASSERT_TRUE(pScanner->waitSpecification(spec));

  EXPECT_EQ(m_nRadiusKm, spec.m_nScanningRadiusKm);
  EXPECT_EQ(m_nMaxUpdateTimeMs, spec.m_nMaxUpdateTimeMs);
}

}

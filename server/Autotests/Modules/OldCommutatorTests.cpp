#include <list>

#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Modules/Fwd.h>
#include <Modules/Managers.h>
#include <Modules/Commutator/Commutator.h>
#include <Network/SessionMux.h>
#include <Utils/Linker.h>

#include <Autotests/TestUtils/Connector.h>
#include <Autotests/Mocks/Modules/MockedCommutator.h>
#include <Autotests/ClientSDK/SyncPipe.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>
#include <Autotests/Mocks/MockedBaseModule.h>
#include <Conveyor/Proceeders.h>

namespace autotests {

// NOTE: why "old"? Because these tests should better use 'ModulesTestFixtures'
class OldCommutatorTests : public ::testing::Test
{
public:
  OldCommutatorTests()
    : m_Conveyor(1),
      m_fConveyorProceeder([this](){ proceedEnviroment(); })
  {}

  void SetUp() override;

private:
  void proceedEnviroment();

protected:
  // Components on server side
  conveyor::Conveyor            m_Conveyor;
  std::function<void()>         m_fConveyorProceeder;
  modules::CommutatorManagerPtr m_pCommutatorManager;
  network::SessionMuxPtr        m_pSessionMux;
  modules::CommutatorPtr        m_pCommutatator;

  // Component, that connects client and server sides
  PlayerConnectorPtr            m_pConnection;

  // Components on client side
  client::PlayerPipePtr         m_pRootPipe;
  client::RouterPtr             m_pRouter;
  client::ClientCommutatorPtr   m_pClient;

  utils::Linker                 m_linker;
};

void OldCommutatorTests::SetUp()
{
  //   ?  ?  ?
  //   |  |  |
  // +----------+   +-----------+
  // | pTunnels |   |  pClient  |
  // +----------+   +-----------+
  //      |               |
  //      +-------+-------+
  //              |                                   ?   ?   ?
  //              |                                   |   |   |
  //              |                               +---------------+
  //              |                               |  pCommutator  |
  //              |                                +---------------+
  //              |                                        |
  //      +----------------+                      +---------------+
  //      |    Router      |                      |   SessionMux  |
  //      +----------------+                      +---------------+
  //              |             +-------------+            |
  //              +-------------| pConnection |------------+
  //                            +-------------+

  // Creating components
  m_pCommutatorManager = std::make_shared<modules::CommutatorManager>();
  m_pSessionMux        = std::make_shared<network::SessionMux>();
  m_pCommutatator      = std::make_shared<modules::Commutator>(m_pSessionMux);

  // Components on client side
  m_pRouter            = std::make_shared<client::Router>();
  m_pClient            = std::make_shared<client::ClientCommutator>(m_pRouter);

  // Setting up components
  m_Conveyor.addLogicToChain(m_pCommutatorManager);
  m_pRouter->setProceeder(m_fConveyorProceeder);

  // Create a connection and a root session on server side
  const uint32_t nConnectionId = 3;
  const uint32_t nRootSession =
    m_pSessionMux->addConnection(nConnectionId, m_pCommutatator);
  m_linker.link(m_pSessionMux, m_pCommutatator);

  // Create a component, that will be used to exchange messages
  // between client and server sides
  m_pConnection = std::make_shared<PlayerConnector>();
  m_linker.link(m_pConnection, m_pSessionMux->asTerminal(), m_pRouter);
  m_linker.addCustomUnlinker([this]() { m_pSessionMux->markAllConnectionsAsClosed(); });

  m_pConnection->onNewConnection(nConnectionId, nRootSession);

  // Connect client commutator to server commutator
  m_pClient->attachToChannel(m_pRouter->openSession(nRootSession));
}

void OldCommutatorTests::proceedEnviroment()
{
  m_Conveyor.proceed(10000);
}

//==============================================================================
// Tests
//==============================================================================

TEST_F(OldCommutatorTests, GetSlotsCount)
{
  const uint32_t nExpectedSlotsCount = 16;
  for (uint32_t i = 0; i < nExpectedSlotsCount; ++i) {
    m_pCommutatator->attachModule(std::make_shared<MockedBaseModule>());
  }

  uint32_t nTotalSlots;
  ASSERT_TRUE(m_pClient->getTotalSlots(nTotalSlots));
  EXPECT_EQ(nExpectedSlotsCount, nTotalSlots);
}

TEST_F(OldCommutatorTests, OpenTunnelSuccessCase)
{
  // 1. Attaching nTotalSlots modules to commutator
  // 2. Opening 10 tunnels to each module

  uint32_t nTotalSlots = 16;

  // 1. Attaching 3 modules to commutator
  for (uint32_t nSlotId = 0; nSlotId < nTotalSlots; ++nSlotId) {
    m_pCommutatator->attachModule(std::make_shared<MockedBaseModule>());
  }

  // 2. Opening 10 tunnels to each module
  for (uint32_t nSlotId = 0; nSlotId < nTotalSlots; ++nSlotId) {
    for (uint32_t nCount = 1; nCount <= 10; ++nCount)
      ASSERT_TRUE(m_pClient->openSession(nSlotId))
          << "failed to open tunnel #" << nCount << " for slot #" << nSlotId;
  }
}

TEST_F(OldCommutatorTests, OpenTunnelToNonExistingSlot)
{
  // 1. Attaching nTotalSlots modules to commutator
  // 2. Trying to open tunnel to slot #nTotalSlots (should failed)

  uint32_t nTotalSlots = 4;
  for (uint32_t nSlotId = 0; nSlotId < nTotalSlots; ++nSlotId) {
    m_pCommutatator->attachModule(std::make_shared<MockedBaseModule>());
  }
  ASSERT_TRUE(m_pClient->sendOpenTunnel(nTotalSlots));
  ASSERT_TRUE(m_pClient->waitOpenTunnelFailed());
}

TEST_F(OldCommutatorTests, TunnelingMessage)
{
  // 1. Attaching commutator to MockedCommutator
  // 2. Doing 5 times:
  //   2.1. Opening new tunnel TO mocked commutator
  //   2.2. Opening new tunnel ON mocked commutator (via tunnel from 2.1)

  // 1. Attaching commutator to MockedCommutator
  MockedCommutatorPtr pMockedCommutator = std::make_shared<MockedCommutator>();
  pMockedCommutator->setEnviromentProceeder(m_fConveyorProceeder);
  m_pCommutatator->attachModule(pMockedCommutator);

  // 2. Doing 5 times:
  for (uint32_t nSlotId = 5; nSlotId < 10; ++nSlotId) {
    //   2.1. Opening new tunnel TO mocked commutator
    client::Router::SessionPtr pTunnel = m_pClient->openSession(0);
    ASSERT_TRUE(pTunnel);

    //   2.2. Opening new tunnel ON mocked commutator (via tunnel from 2.1)
    client::ClientCommutatorPtr pAnotherClient =
        std::make_shared<client::ClientCommutator>(m_pRouter);
    pAnotherClient->attachToChannel(pTunnel);
    ASSERT_TRUE(pAnotherClient->sendOpenTunnel(nSlotId));
    ASSERT_TRUE(pMockedCommutator->sendOpenTunnelFailed(
                  pTunnel->sessionId(), spex::ICommutator::INVALID_SLOT));
    ASSERT_TRUE(pAnotherClient->waitOpenTunnelFailed());
  }
}

TEST_F(OldCommutatorTests, TunnelingMessageToOfflineModule)
{
  // 1. Attaching commutator to MockedCommutator
  modules::CommutatorPtr pAnotherCommutator =
    std::make_shared<modules::Commutator>(m_pSessionMux);
  m_pCommutatator->attachModule(pAnotherCommutator);

  // 2. Opening tunnel to mocked commutator
  client::Router::SessionPtr pTunnel = m_pClient->openSession(0);
  ASSERT_TRUE(pTunnel);

  // 3. Put mocked commutator to offline and send any command
  client::ClientCommutatorPtr pAnotherClient =
      std::make_shared<client::ClientCommutator>(m_pRouter);
  pAnotherClient->attachToChannel(pTunnel);
  pAnotherCommutator->putOffline();
  ASSERT_TRUE(pAnotherClient->sendOpenTunnel(1));
  ASSERT_TRUE(pTunnel->waitCloseTunnelInd());
}

TEST_F(OldCommutatorTests, CloseTunnel)
{
  // 1. Attaching commutator to MockedCommutator
  MockedCommutatorPtr pMockedCommutator = std::make_shared<MockedCommutator>();
  m_pCommutatator->attachModule(pMockedCommutator);
  pMockedCommutator->setEnviromentProceeder(m_fConveyorProceeder);

  // 1. Open tunnel
  client::Router::SessionPtr pTunnel = m_pClient->openSession(0);
  ASSERT_TRUE(pTunnel);

  // 2. Try to send some request
  client::ClientCommutatorPtr pAnotherClient =
      std::make_shared<client::ClientCommutator>(m_pRouter);
  pAnotherClient->attachToChannel(pTunnel);

  ASSERT_TRUE(pAnotherClient->sendTotalSlotsReq());
  uint32_t nTotalSlots = 16;
  ASSERT_TRUE(pMockedCommutator->waitTotalSlotsReq(pTunnel->sessionId()));
  ASSERT_TRUE(pMockedCommutator->sendTotalSlots(pTunnel->sessionId(), nTotalSlots));
  ASSERT_TRUE(pAnotherClient->waitTotalSlots(nTotalSlots));

  // 3. close channel
  ASSERT_TRUE(m_pClient->closeTunnel(pTunnel));
  ASSERT_TRUE(pTunnel->waitCloseTunnelInd());

  // 4. try to send yet another request (should fail)
  // ASSERT_TRUE(pAnotherClient->sendTotalSlotsReq());
  // ASSERT_FALSE(pMockedCommutator->waitTotalSlotsReq(pTunnel->sessionId()));
}

} // namespace autotests

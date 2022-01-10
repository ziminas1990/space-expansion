#include <list>

#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Modules/Commutator/Commutator.h>
#include <Modules/Commutator/CommutatorManager.h>
#include <Autotests/TestUtils/BidirectionalChannel.h>
#include <Autotests/TestUtils/SessionsMux.h>
#include <Autotests/Mocks/Modules/MockedCommutator.h>

#include <Autotests/ClientSDK/SyncPipe.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>
#include <Autotests/Mocks/MockedBaseModule.h>
#include <Conveyor/Proceeders.h>

namespace autotests {

class CommutatorTests : public ::testing::Test
{
public:
  CommutatorTests()
    : m_Conveyor(1),
      m_fConveyorProceeder([this](){ proceedEnviroment(); })
  {}

  void SetUp() override;
  void TearDown() override;

private:
  void proceedEnviroment();

protected:
  // Components on server side
  conveyor::Conveyor            m_Conveyor;
  std::function<void()>         m_fConveyorProceeder;
  modules::CommutatorManagerPtr m_pCommutatorManager;
  modules::CommutatorPtr        m_pCommutatator;

  // Component, that connects client and server sides
  BidirectionalChannelPtr       m_pChannel;

  // Components on client side
  SessionMuxPtr                 m_pSessionMux;
  client::PlayerPipePtr         m_pProtobufPipe;
  client::ClientCommutatorPtr   m_pClient;
};

void CommutatorTests::SetUp()
{
  //   ?  ?  ?
  //   |  |  |
  // +----------+   +-----------+
  // | pTunnels |   |  pClient  |
  // +----------+   +-----------+
  //      |               |
  //      +-------+-------+
  //              |
  //      +----------------+
  //      | pProtobufPipe  |
  //      +----------------+                          ?   ?   ?
  //              |                                   |   |   |
  //      +----------------+                      +---------------+
  //      |   pSessionMux  |                      |  pCommutator  |
  //      +----------------+                      +---------------+
  //              |              +----------+             |
  //              +--------------| pChannel |-------------+
  //                             +----------+

  // Creating components
  m_pCommutatorManager = std::make_shared<modules::CommutatorManager>();
  m_pCommutatator      = std::make_shared<modules::Commutator>();
  m_pChannel           = std::make_shared<BidirectionalChannel>();

  // Components on client side
  m_pSessionMux        = std::make_shared<SessionMux>();
  m_pProtobufPipe      = std::make_shared<client::PlayerPipe>();
  m_pClient            = std::make_shared<client::ClientCommutator>();

  // Setting up components
  m_Conveyor.addLogicToChain(m_pCommutatorManager);
  m_pProtobufPipe->setProceeder(m_fConveyorProceeder);

  // Linking components
  m_pChannel->link(m_pSessionMux, m_pCommutatator);
  m_pSessionMux->openSession(1, m_pProtobufPipe);
  m_pClient->attachToChannel(m_pProtobufPipe);
}

void CommutatorTests::TearDown()
{
  m_pProtobufPipe->detachDownlevel();
  m_pSessionMux->closeSession(1);
  m_pChannel->unlink();
  m_pCommutatator->detachFromModules();
  m_pCommutatator->detachFromChannel();
}

void CommutatorTests::proceedEnviroment()
{
  m_Conveyor.proceed(10000);
}

//==============================================================================
// Tests
//==============================================================================

TEST_F(CommutatorTests, GetSlotsCount)
{
  uint32_t nExpectedSlotsCount = 16;
  for (uint32_t i = 0; i < nExpectedSlotsCount; ++i) {
    m_pCommutatator->attachModule(std::make_shared<MockedBaseModule>());
  }
  ASSERT_TRUE(m_pClient->getTotalSlots(nExpectedSlotsCount));
}

TEST_F(CommutatorTests, OpenTunnelSuccessCase)
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
      ASSERT_TRUE(m_pClient->openTunnel(nSlotId))
          << "failed to open tunnel #" << nCount << " for slot #" << nSlotId;
  }
}

TEST_F(CommutatorTests, OpenTunnelToNonExistingSlot)
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

TEST_F(CommutatorTests, TunnelingMessage)
{
  // 1. Attaching commutator to MockedCommutator
  // 2. Doing 5 times:
  //   2.1. Opening new tunnel TO mocked commutator
  //   2.2. Opening new tunnel ON mocked commutator (via tunnel from 2.1)

  // 1. Attaching commutator to MockedCommutator
  MockedCommutatorPtr pMockedCommutator = std::make_shared<MockedCommutator>();
  pMockedCommutator->setEnviromentProceeder(m_fConveyorProceeder);
  pMockedCommutator->attachToChannel(m_pCommutatator);
  m_pCommutatator->attachModule(pMockedCommutator);

  // 2. Doing 5 times:
  for(uint32_t nSlotId = 5; nSlotId < 10; ++nSlotId) {
    //   2.1. Opening new tunnel TO mocked commutator
    client::TunnelPtr pTunnel = m_pClient->openTunnel(0);
    ASSERT_TRUE(pTunnel);

    //   2.2. Opening new tunnel ON mocked commutator (via tunnel from 2.1)
    client::ClientCommutatorPtr pAnotherClient =
        std::make_shared<client::ClientCommutator>();
    pAnotherClient->attachToChannel(pTunnel);
    ASSERT_TRUE(pAnotherClient->sendOpenTunnel(nSlotId));
    ASSERT_TRUE(pMockedCommutator->waitOpenTunnel(pTunnel->getTunnelId(), nSlotId));
    ASSERT_TRUE(pMockedCommutator->sendOpenTunnelFailed(
                  pTunnel->getTunnelId(), spex::ICommutator::INVALID_SLOT));
    ASSERT_TRUE(pAnotherClient->waitOpenTunnelFailed());
  }
}

TEST_F(CommutatorTests, TunnelingMessageToOfflineModule)
{
  // 1. Attaching commutator to MockedCommutator
  MockedCommutatorPtr pMockedCommutator = std::make_shared<MockedCommutator>();
  pMockedCommutator->attachToChannel(m_pCommutatator);
  m_pCommutatator->attachModule(pMockedCommutator);
  pMockedCommutator->setEnviromentProceeder(m_fConveyorProceeder);

  // 2. Opening tunnel to mocked commutator
  client::TunnelPtr pTunnel = m_pClient->openTunnel(0);
  ASSERT_TRUE(pTunnel);

  // 3. Put mocked commutator to offline and sending any command
  client::ClientCommutatorPtr pAnotherClient =
      std::make_shared<client::ClientCommutator>();
  pAnotherClient->attachToChannel(pTunnel);
  pMockedCommutator->putOffline();
  ASSERT_TRUE(pAnotherClient->sendOpenTunnel(1));
  ASSERT_FALSE(pMockedCommutator->waitAny(pTunnel->getTunnelId(), 50));
}

TEST_F(CommutatorTests, CloseTunnel)
{
  // 1. Attaching commutator to MockedCommutator
  MockedCommutatorPtr pMockedCommutator = std::make_shared<MockedCommutator>();
  pMockedCommutator->attachToChannel(m_pCommutatator);
  m_pCommutatator->attachModule(pMockedCommutator);
  pMockedCommutator->setEnviromentProceeder(m_fConveyorProceeder);

  // 1. Open tunnel
  client::TunnelPtr pTunnel = m_pClient->openTunnel(0);
  ASSERT_TRUE(pTunnel);

  // 2. Try to send some request
  client::ClientCommutatorPtr pAnotherClient =
      std::make_shared<client::ClientCommutator>();
  pAnotherClient->attachToChannel(pTunnel);

  ASSERT_TRUE(pAnotherClient->sendTotalSlotsReq());
  uint32_t nTotalSlots = 16;
  ASSERT_TRUE(pMockedCommutator->waitTotalSlotsReq(pTunnel->getTunnelId()));
  ASSERT_TRUE(pMockedCommutator->sendTotalSlots(pTunnel->getTunnelId(), nTotalSlots));
  ASSERT_TRUE(pAnotherClient->waitTotalSlots(nTotalSlots));

  // 3. close channel
  ASSERT_TRUE(m_pClient->closeTunnel(pTunnel));
  spex::ICommutator closeInd;
  ASSERT_TRUE(pTunnel->wait(closeInd));
  ASSERT_EQ(spex::ICommutator::kCloseTunnelInd, closeInd.choice_case());

  // 4. try to send yet another request (should fail)
  ASSERT_TRUE(pAnotherClient->sendTotalSlotsReq());
  ASSERT_FALSE(pMockedCommutator->waitTotalSlotsReq(pTunnel->getTunnelId()));
}

} // namespace autotests

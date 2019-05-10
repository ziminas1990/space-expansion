#include "CommutatorTests.h"
#include <Autotests/Mocks/MockedBaseModule.h>
#include <Conveyor/Proceeders.h>

namespace autotests {

CommutatorTests::CommutatorTests()
  : m_Conveyor(1),
    m_fConveyorProceeder([this](){ proceedEnviroment(); }),
    m_nMainSessionId(1)
{

}

void CommutatorTests::SetUp()
{
  //   ?  ?  ?
  //   |  |  |
  // +----------+   +-----------+
  // | pTunnels |   |  pClient  |
  // +----------+   +-----------+
  //      |               |
  //      +-------+-------+                           ?   ?   ?
  //              |                                   |   |   |
  //      +----------------+                     ++===============++
  //      | pProtobufPipe  |                     ||  pCommutator  ||
  //      +----------------+                     ++===============++
  //              |                                       |
  //              |              +----------+             |
  //              +--------------| pChannel |-------------+
  //                             +----------+

  // Creating components
  m_pCommutatorManager = std::make_shared<modules::CommutatorManager>();
  m_pCommutatator      = m_pCommutatorManager->makeCommutator();
  m_pChannel           = std::make_shared<BidirectionalChannel>();
  m_pProtobufPipe      = std::make_shared<ProtobufSyncPipe>();
  m_pClient            = std::make_shared<CommutatorClient>(m_nMainSessionId);
  m_pTunnels           = std::make_shared<ProtobufTunnel>(m_nMainSessionId);

  // Setting up components
  m_Conveyor.addLogicToChain(m_pCommutatorManager);
  m_pProtobufPipe->setEnviromentProceeder(m_fConveyorProceeder);
  m_pTunnels->setEnviromentProceeder(m_fConveyorProceeder);

  // Linking components
  m_pCommutatator->attachToChannel(m_pChannel);
  m_pChannel->attachToTerminal(m_pCommutatator);
  m_pProtobufPipe->attachToChannel(m_pChannel);
  m_pChannel->createNewSession(m_nMainSessionId, m_pProtobufPipe);
  m_pClient->attachToSyncChannel(m_pProtobufPipe);
  m_pProtobufPipe->attachToTunnel(m_nMainSessionId, m_pTunnels);
  m_pTunnels->attachToChannel(m_pProtobufPipe);
}

void CommutatorTests::TearDown()
{
  m_pProtobufPipe->detachFromTerminal();
  m_pCommutatator->detachFromChannel();
}

void CommutatorTests::proceedEnviroment()
{
  m_Conveyor.proceed(10000);
}

//========================================================================================
// Tests
//========================================================================================

TEST_F(CommutatorTests, GetSlotsCount)
{
  uint32_t nExpectedSlotsCount = 16;
  for (uint32_t i = 0; i < nExpectedSlotsCount; ++i) {
    m_pCommutatator->attachModule(std::make_shared<MockedBaseModule>());
  }
  ASSERT_TRUE(m_pClient->sendGetTotalSlots(nExpectedSlotsCount));
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
  ASSERT_TRUE(m_pClient->openTunnel(nTotalSlots, false));
}

TEST_F(CommutatorTests, TunnelingMessage)
{
  // 1. Attaching commutator to MockedCommutator
  // 2. Doing 5 times:
  //   2.1. Opening new tunnel TO mocked commutator
  //   2.2. Opening new tunnel ON mocked commutator (via tunnel from 2.1)

  // 1. Attaching commutator to MockedCommutator
  MockedCommutatorPtr pMockedCommutator = std::make_shared<MockedCommutator>();
  pMockedCommutator->attachToChannel(m_pCommutatator);
  m_pCommutatator->attachModule(pMockedCommutator);
  pMockedCommutator->setEnviromentProceeder(m_fConveyorProceeder);

  // 2. Doing 5 times:
  for(uint32_t nSlotId = 5; nSlotId < 10; ++nSlotId) {
    //   2.1. Opening new tunnel TO mocked commutator
    uint32_t nTunnelId = 0;
    ASSERT_TRUE(m_pClient->openTunnel(0, true, &nTunnelId));

    //   2.2. Opening new tunnel ON mocked commutator (via tunnel from 2.1)
    CommutatorClientPtr pAnotherClient = std::make_shared<CommutatorClient>(nTunnelId);
    pAnotherClient->attachToSyncChannel(m_pTunnels);
    ASSERT_TRUE(pAnotherClient->sendOpenTunnel(nSlotId));
    ASSERT_TRUE(pMockedCommutator->waitOpenTunnel(nTunnelId, nSlotId));
    ASSERT_TRUE(pMockedCommutator->sendOpenTunnelFailed(nTunnelId));
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
  uint32_t nTunnelId = 0;
  ASSERT_TRUE(m_pClient->openTunnel(0, true, &nTunnelId));

  // 3. Put mocked commutator to offline and sending any command
  CommutatorClientPtr pAnotherClient = std::make_shared<CommutatorClient>(nTunnelId);
  pAnotherClient->attachToSyncChannel(m_pTunnels);
  pMockedCommutator->putOffline();
  ASSERT_TRUE(pAnotherClient->sendOpenTunnel(1));
  ASSERT_FALSE(pMockedCommutator->waitAny(nTunnelId, 50));
}

} // namespace autotests

#include "CommutatorTests.h"
#include <Autotests/Mocks/MockedBaseModule.h>
#include <Conveyor/Proceeders.h>

namespace autotests {

void CommutatorTests::SetUp()
{
  // Creating components
  m_pCommutatorManager = std::make_shared<modules::CommutatorManager>();
  m_pCommutatator      = m_pCommutatorManager->makeCommutator();
  m_pClient            = std::make_shared<CommutatorClient>();
  m_pProtobufPipe      = std::make_shared<ProtobufSyncPipe>();
  m_pChannel           = std::make_shared<BidirectionalChannel>();

  // Setting up components
  conveyor.addLogicToChain(m_pCommutatorManager);
  m_pProtobufPipe->setEnviromentProceeder([this](){ conveyor.proceed(10000); });

  // Linking components
  m_pClient->attachToSyncChannel(m_pProtobufPipe);
  m_pCommutatator->attachToChannel(m_pChannel);
  m_pChannel->attachToTerminal(m_pCommutatator);
  m_pProtobufPipe->attachToChannel(m_pChannel);
  m_pChannel->createNewSession(1, m_pProtobufPipe);
}

void CommutatorTests::TearDown()
{
  m_pProtobufPipe->detachFromTerminal();
  m_pCommutatator->detachFromChannel();
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
  ASSERT_TRUE(m_pClient->sendGetTotalSlots(1, nExpectedSlotsCount));
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
      ASSERT_TRUE(m_pClient->openTunnel(1, nSlotId))
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
  ASSERT_TRUE(m_pClient->openTunnel(1, nTotalSlots, false));
}

} // namespace autotests
#pragma once

#include <list>

#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Modules/Commutator/Commutator.h>
#include <Modules/Commutator/CommutatorManager.h>
#include <Autotests/TestUtils/ProtobufSyncPipe.h>
#include <Autotests/TestUtils/BidirectionalChannel.h>
#include <Autotests/TestUtils/ProtobufTunnel.h>
#include <Autotests/Mocks/Modules/MockedCommutator.h>

namespace autotests
{

class CommutatorTests : public ::testing::Test
{
public:
  CommutatorTests();

  void SetUp() override;
  void TearDown() override;

private:
  void proceedEnviroment();

protected:
  conveyor::Conveyor            m_Conveyor;
  std::function<void()>         m_fConveyorProceeder;
  modules::CommutatorManagerPtr m_pCommutatorManager;
  modules::CommutatorPtr        m_pCommutatator;
  CommutatorClientPtr           m_pClient;
  ProtobufSyncPipePtr           m_pProtobufPipe;
  BidirectionalChannelPtr       m_pChannel;
  ProtobufTunnelPtr             m_pTunnels;

  uint32_t m_nMainSessionId;
};

} // namespace autotests

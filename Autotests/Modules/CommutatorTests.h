#pragma once

#include <list>

#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Modules/Commutator/Commutator.h>
#include <Modules/Commutator/CommutatorManager.h>
#include <Autotests/TestUtils/ProtobufSyncPipe.h>
#include <Autotests/TestUtils/BidirectionalChannel.h>
#include <Autotests/Mocks/Modules/MockedCommutator.h>

#include <Autotests/ClientSDK/SyncPipe.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>

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
  std::function<void()>         m_fConveyorProceeder;

  // Components on server side
  conveyor::Conveyor            m_Conveyor;
  modules::CommutatorManagerPtr m_pCommutatorManager;
  modules::CommutatorPtr        m_pCommutatator;

  // Component, that connects client and server sides
  BidirectionalChannelPtr       m_pChannel;

  // Components on client side
  client::ClientCommutatorPtr   m_pClient;
  client::SyncPipePtr           m_pProtobufPipe;
};

} // namespace autotests

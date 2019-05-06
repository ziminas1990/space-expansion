#pragma once

#include <gtest/gtest.h>

#include <Conveyor/Conveyor.h>
#include <Modules/Commutator/Commutator.h>
#include <Modules/Commutator/CommutatorManager.h>
#include <Autotests/Mocks/ProtobufSyncPipe.h>
#include <Autotests/Mocks/Modules/MockedCommutator.h>

namespace autotests
{

class CommutatorTests : public ::testing::Test
{
public:
  CommutatorTests() : conveyor(1) {}

  void SetUp() override;
  void TearDown() override;

protected:
  conveyor::Conveyor            conveyor;
  modules::CommutatorManagerPtr m_pCommutatorManager;
  modules::CommutatorPtr        m_pCommutatator;
  ProtobufSyncPipePtr           m_pProtobufPipe;
  CommutatorClientPtr           m_pClient;
};

} // namespace autotests

#pragma once

#include <memory>
#include <Protocol.pb.h>
#include <Autotests/TestUtils/ProtobufSyncPipe.h>
#include <Modules/BaseModule.h>

namespace autotests {

class CommutatorClient
{
public:
  virtual ~CommutatorClient() = default;

  void attachToSyncChannel(ProtobufSyncPipePtr pSyncPipe)
  { m_pSyncPipe = pSyncPipe; }

  // Sending GetTotalSlots request and checking response
  bool sendGetTotalSlots(uint32_t nSessionId, uint32_t nExpectedSlots);

  bool openTunnel(uint32_t nSessionId, uint32_t nSlotId, bool lExpectSuccess = true,
                  uint32_t *pOpenedTunnelId = nullptr);

private:
  ProtobufSyncPipePtr m_pSyncPipe;
};


class MockedCommutator :
    public ProtobufSyncPipe,
    public modules::BaseModule
{
public:
  MockedCommutator();

  bool waitOpenTunnel(uint32_t nSessionId, uint32_t nSlotId);
};

using CommutatorClientPtr = std::shared_ptr<CommutatorClient>;

} // namespace autotests

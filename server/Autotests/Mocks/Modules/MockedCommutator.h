#pragma once

#include <memory>
#include <Protocol.pb.h>
#include <Autotests/TestUtils/ProtobufSyncPipe.h>
#include <Autotests/Mocks/MockedBaseModule.h>
#include <Modules/BaseModule.h>

namespace autotests {

class MockedCommutator : public MockedBaseModule
{
public:
  MockedCommutator();

  bool waitOpenTunnel(uint32_t nSessionId, uint32_t nSlotId);
  bool sendOpenTunnelSuccess(uint32_t nSessionId, uint32_t nTunnelId);
  bool sendOpenTunnelFailed(uint32_t nSessionId, spex::ICommutator::Status eReason);
};

using MockedCommutatorPtr = std::shared_ptr<MockedCommutator>;

} // namespace autotests

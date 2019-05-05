#pragma once

#include <memory>
#include <queue>
#include <functional>
#include <Modules/BaseModule.h>
#include "ProtobufSyncPipe.h"

namespace autotests
{

class MockedBaseModule :
    public modules::BaseModule,
    public ProtobufSyncPipe
{
public:

  MockedBaseModule()
    : modules::BaseModule("MockedBaseModule"),
      ProtobufSyncPipe(ProtobufSyncPipe::eMockedTerminalMode)
  {}

  // override from ITerminal interface
  void onMessageReceived(uint32_t nSessionId, spex::ICommutator&& frame) override
  {
    ProtobufSyncPipe::onMessageReceived(nSessionId, std::move(frame));
  }

  void attachToChannel(network::IProtobufChannelPtr /*pChannel*/) override {}
};

using MockedBaseModulePtr = std::shared_ptr<MockedBaseModule>;

} // namespace autotests

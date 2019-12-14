#pragma once

#include <memory>
#include <queue>
#include <functional>
#include <Modules/BaseModule.h>
#include <Autotests/TestUtils/ProtobufSyncPipe.h>

namespace autotests
{

class MockedBaseModule :
    public modules::BaseModule,
    public ProtobufSyncPipe
{
public:
  MockedBaseModule(std::string&& sModuleType = "MockedBaseModule")
    : modules::BaseModule(std::move(sModuleType), std::string())
  {}

  // We should call only ProtobufSyncPipe implementation of this functions
  // overrides from ITerminal interface
  bool openSession(uint32_t nSessionId) override
  {
    return ProtobufSyncPipe::openSession(nSessionId);
  }

  void onMessageReceived(uint32_t nSessionId, spex::Message const& frame) override
  {
    ProtobufSyncPipe::onMessageReceived(nSessionId, frame);
  }

  void onSessionClosed(uint32_t nSessionId) override
  {
    ProtobufSyncPipe::onSessionClosed(nSessionId);
  }

  void attachToChannel(network::IProtobufChannelPtr pChannel) override
  {
    ProtobufSyncPipe::attachToChannel(pChannel);
    modules::BaseModule::attachToChannel(pChannel);
  }

  void detachFromChannel() override
  {
    ProtobufSyncPipe::detachFromChannel();
    modules::BaseModule::detachFromChannel();
  }
};

using MockedBaseModulePtr = std::shared_ptr<MockedBaseModule>;

} // namespace autotests

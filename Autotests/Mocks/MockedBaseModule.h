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
};

using MockedBaseModulePtr = std::shared_ptr<MockedBaseModule>;

} // namespace autotests

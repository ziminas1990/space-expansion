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

  MockedBaseModule()
    : modules::BaseModule("MockedBaseModule")
  {}
};

using MockedBaseModulePtr = std::shared_ptr<MockedBaseModule>;

} // namespace autotests

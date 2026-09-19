#pragma once

#include <memory>

#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <Protocol.pb.h>

namespace autotests { namespace client {

class Game : public ClientBaseModule
{
public:
  bool monitor();
  bool waitGameOverReport(spex::IGame::GameOver& report, uint16_t nTimeout = 500);
};

using GamePtr = std::shared_ptr<Game>;

}}  // namespace autotests::client

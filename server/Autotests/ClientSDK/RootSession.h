#pragma once

#include <memory>

#include <Autotests/ClientSDK/SyncPipe.h>

namespace autotests::client {

class RootSession {
private:
  PlayerPipePtr m_pChannel;

public:

  void attachToChannel(PlayerPipePtr pChannel) { m_pChannel = pChannel; }
  void detachChannel() { m_pChannel.reset(); }

  bool openCommutatorSession(uint32_t& nSessionId);

  bool close();
  bool waitCloseInd();
};

using RootSessionPtr = std::shared_ptr<RootSession>;

}  // autotests::client
#pragma once

#include <Autotests/ClientSDK/SyncPipe.h>

namespace autotests { namespace client {

class ClientBaseModule
{
public:
  void attachToChannel(SyncPipePtr pSyncPipe) { m_pSyncPipe = pSyncPipe; }
  void detachChannel() { m_pSyncPipe.reset(); }
  SyncPipePtr getChannel() { return m_pSyncPipe; }

  // Forwarding interface from SyncPipe
  bool send(spex::Message const& message)
  { return m_pSyncPipe && m_pSyncPipe->send(message); }

  template<typename MessageType>
  bool wait(MessageType& message, uint16_t nTimeout = 500)
  { return m_pSyncPipe && m_pSyncPipe->wait(message, nTimeout); }

private:
  SyncPipePtr m_pSyncPipe;
};

}} // namespace autotests

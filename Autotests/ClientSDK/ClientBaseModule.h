#pragma once

#include <Autotests/ClientSDK/SyncPipe.h>

namespace autotests { namespace client {

class ClientBaseModule
{
public:
  void attachToChannel(SyncPipePtr pSyncPipe) { m_pSyncPipe = pSyncPipe; }
  void detachChannel() { m_pSyncPipe.reset(); }
  SyncPipePtr getChannel() { return m_pSyncPipe; }

private:
  SyncPipePtr m_pSyncPipe;
};

}} // namespace autotests

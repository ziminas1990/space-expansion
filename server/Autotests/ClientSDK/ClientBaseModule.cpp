#include "ClientBaseModule.h"

namespace autotests { namespace client {

void ClientBaseModule::dropQueuedMessage()
{
  if (m_pSyncPipe) {
    m_pSyncPipe->dropAll();
  }
}

}}

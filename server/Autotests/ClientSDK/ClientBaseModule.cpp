#include "ClientBaseModule.h"

namespace autotests { namespace client {

void ClientBaseModule::dropQueuedMessage()
{
  if (m_pSyncPipe) {
    m_pSyncPipe->dropAll();
  }
}

bool ClientBaseModule::waitCloseInd()
{
  spex::ISessionControl response;
  return wait(response)
      && response.choice_case() != spex::ISessionControl::kClosedInd;
}

}}  // namespace autotests::client

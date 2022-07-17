#include "ClientBaseModule.h"

namespace autotests { namespace client {

void ClientBaseModule::dropQueuedMessage()
{
  if (m_pSyncPipe) {
    m_pSyncPipe->dropAll();
  }
}

bool ClientBaseModule::disconnect()
{
  spex::Message request;
  request.mutable_session()->set_close(true);
  return send(std::move(request)) && waitCloseInd();
}

bool ClientBaseModule::waitCloseInd()
{
  spex::ISessionControl response;
  return wait(response)
      && response.choice_case() == spex::ISessionControl::kClosedInd;
}

}}  // namespace autotests::client

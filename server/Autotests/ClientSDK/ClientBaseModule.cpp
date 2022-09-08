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

bool ClientBaseModule::waitCloseInd(uint16_t nTimeoutMs)
{
  spex::ISessionControl response;
  return wait(response, nTimeoutMs)
      && response.choice_case() == spex::ISessionControl::kClosedInd;
}

}}  // namespace autotests::client

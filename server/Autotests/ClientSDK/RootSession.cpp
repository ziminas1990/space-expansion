#include "RootSession.h"
#include "Protocol.pb.h"

namespace autotests::client {

bool RootSession::openCommutatorSession(uint32_t& nSessionId)
{
  spex::Message request;
  spex::IRootSession* pBody = request.mutable_root_session();
  pBody->set_new_commutator_session(true);

  if (!m_pChannel->send(std::move(request))) {
    return false;
  }

  spex::IRootSession response;
  if (!m_pChannel->wait(response)) {
    return false;
  }

  if (response.choice_case() != spex::IRootSession::kCommutatorSession) {
    return false;
  }

  nSessionId = response.commutator_session();
  return true;
}

bool RootSession::close()
{
  spex::Message request;
  request.mutable_session()->set_close(true);
  return m_pChannel->send(std::move(request)) && waitCloseInd();
}

bool RootSession::waitCloseInd()
{
  spex::ISessionControl response;
  return m_pChannel->wait(response)
      && response.choice_case() == spex::ISessionControl::kClosedInd;
}

} // namespace autotests::client


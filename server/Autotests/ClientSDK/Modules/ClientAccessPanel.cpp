#include "ClientAccessPanel.h"

namespace autotests { namespace client {

bool ClientAccessPanel::login(
    std::string const& sLogin, 
    std::string const& sPassword,
    uint16_t&          nRemotePort,
    uint32_t&          nSessionId)
{
  return sendLoginRequest(sLogin, sPassword)
      && waitLoginSuccess(nRemotePort, nSessionId);
}

bool ClientAccessPanel::sendLoginRequest(
    std::string const& sLogin, std::string const& sPassword)
{
  spex::Message message;
  spex::IAccessPanel::LoginRequest *pLoginReq =
      message.mutable_accesspanel()->mutable_login();
  pLoginReq->set_login(sLogin);
  pLoginReq->set_password(sPassword);
  return send(std::move(message));
}

bool ClientAccessPanel::waitLoginSuccess(uint16_t& nServerPort,
                                         uint32_t& nSessionId)
{
  spex::IAccessPanel response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IAccessPanel::kAccessGranted)
    return false;
  const spex::IAccessPanel::AccessGranted& granted = response.access_granted();
  nServerPort = static_cast<uint16_t>(granted.port());
  nSessionId = static_cast<uint32_t>(granted.session_id());
  return true;
}

bool ClientAccessPanel::waitLoginFailed()
{
  spex::IAccessPanel response;
  return wait(response)
      && response.choice_case() == spex::IAccessPanel::kAccessRejected;
}

}}  // namespace autotests::client

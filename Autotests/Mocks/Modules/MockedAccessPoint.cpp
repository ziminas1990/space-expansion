#include "MockedAccessPoint.h"

#include <Protocol.pb.h>

namespace autotests
{

bool AccessPointClient::sendLoginRequest(
    std::string const& sLogin, std::string const& sPassword,
    std::string const& sIP, uint16_t nPort)
{
  spex::Message message;
  spex::IAccessPanel::LoginRequest *pLoginReq =
      message.mutable_accesspanel()->mutable_login();
  pLoginReq->set_login(sLogin);
  pLoginReq->set_password(sPassword);
  pLoginReq->set_ip(sIP);
  pLoginReq->set_port(nPort);
  return m_pSyncPipe->send(0, message);
}

bool AccessPointClient::waitLoginSuccess(uint16_t &nServerPort)
{
  spex::IAccessPanel response;
  if (!m_pSyncPipe->wait(0, response))
    return false;
  if (response.choice_case() != spex::IAccessPanel::kLoginSuccess)
    return false;
  nServerPort = uint16_t(response.loginsuccess().port());
  return true;
}

bool AccessPointClient::waitLoginFailed()
{
  spex::IAccessPanel response;
  return m_pSyncPipe->wait(0, response)
      && response.choice_case() == spex::IAccessPanel::kLoginFailed;
}

} // namespace autotests

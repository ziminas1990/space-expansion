#include "ClientAccessPanel.h"

namespace autotests { namespace client {

bool ClientAccessPanel::login(
    std::string const& sLogin, std::string const& sPassword,
    std::string const& sLocalIP, uint16_t nLocalPort,
    uint16_t &nRemotePort)
{
  return sendLoginRequest(sLogin, sPassword, sLocalIP, nLocalPort)
      && waitLoginSuccess(nRemotePort);
}

bool ClientAccessPanel::sendLoginRequest(
    std::string const& sLogin, std::string const& sPassword,
    std::string const& sLocalIP, uint16_t nLocalPort)
{
  spex::Message message;
  spex::IAccessPanel::LoginRequest *pLoginReq =
      message.mutable_accesspanel()->mutable_login();
  pLoginReq->set_login(sLogin);
  pLoginReq->set_password(sPassword);
  pLoginReq->set_ip(sLocalIP);
  pLoginReq->set_port(nLocalPort);
  return m_pSyncPipe->send(message);
}

bool ClientAccessPanel::waitLoginSuccess(uint16_t &nServerPort)
{
  spex::IAccessPanel response;
  if (!m_pSyncPipe->wait(response))
    return false;
  if (response.choice_case() != spex::IAccessPanel::kLoginSuccess)
    return false;
  nServerPort = uint16_t(response.loginsuccess().port());
  return true;
}

bool ClientAccessPanel::waitLoginFailed()
{
  spex::IAccessPanel response;
  return m_pSyncPipe->wait(response)
      && response.choice_case() == spex::IAccessPanel::kLoginFailed;
}

}}  // namespace autotests::client

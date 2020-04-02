#include "ClientAdministratorPanel.h"

namespace autotests { namespace client {

AdministratorPanel::Status AdministratorPanel::login(
    std::string const& sLogin, std::string const& sPassword, uint64_t& nToken)
{
  admin::Message message;
  admin::Access::Login* pBody = message.mutable_access()->mutable_login();
  pBody->set_login(sLogin);
  pBody->set_password(sPassword);

  if (!m_pPrivilegedPipe->send(message)) {
    return eTransportError;
  }

  admin::Access response;
  if (!m_pPrivilegedPipe->wait(response)) {
    return eTimeoutError;
  }

  switch (response.choice_case()) {
    case admin::Access::kSuccess:
      nToken = response.success();
      return eSuccess;
    case admin::Access::kFail:
      return eLoginFailed;
    default:
      return eUnexpectedMessage;
  }
}

}}  // namespace autotests::client

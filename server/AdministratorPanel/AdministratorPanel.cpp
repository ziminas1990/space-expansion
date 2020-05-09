#include "AdministratorPanel.h"
#include <SystemManager.h>
#include <random>

AdministratorPanel::AdministratorPanel(config::IAdministratorCfg const& cfg,
                                       unsigned int nTokenPattern)
  : m_cfg(cfg), m_tokenGenerator(nTokenPattern)
{
}

void AdministratorPanel::attachToSystemManager(SystemManager* pSystemManager)
{
  assert(getChannel() != nullptr);  // must be attached to channel first!

  m_pSystemManager = pSystemManager;
  m_clockControl.setup(m_pSystemManager, getChannel());
  m_screen.setup(m_pSystemManager, getChannel());
}

bool AdministratorPanel::prephare(uint16_t nStageId, uint32_t, uint64_t)
{
  assert(nStageId == 0);
  handleBufferedMessages();
  m_clockControl.proceed();
  m_screen.proceed();
  // This logic shouldn't be run in multithreading mode
  return false;
}

void AdministratorPanel::handleMessage(uint32_t nSessionId, admin::Message const& message)
{
  if (message.token() == 0 && message.choice_case() == admin::Message::kAccess) {
    if (message.access().choice_case() != admin::Access::kLogin) {
      closeSession(nSessionId);
      return;
    }
    onLoginRequest(nSessionId, message.access().login());
    return;
  }

  std::map<uint32_t, uint64_t>::const_iterator I = m_tokens.find(nSessionId);
  if (I == m_tokens.end() || message.token() != I->second) {
    // invalid token -> ignoring message
    return;
  }

  switch(message.choice_case()) {
    case admin::Message::kSystemClock:
      m_clockControl.handleMessage(nSessionId, message.system_clock());
      return;
    case admin::Message::kScreen:
      m_screen.handleMessage(nSessionId, message.screen());
      return;
    default:
      return;
  }
}

void AdministratorPanel::onLoginRequest(uint32_t nSessionId,
                                        admin::Access::Login const& message)
{
  if (message.login() != m_cfg.getLogin() || message.password() != m_cfg.getPassword()) {
    sendLoginFailed(nSessionId);
    closeSession(nSessionId);
    return;
  }

  uint64_t nToken = m_tokenGenerator.yield();
  m_tokens[nSessionId] = nToken;
  sendLoginSuccess(nSessionId, nToken);
}

void AdministratorPanel::sendLoginSuccess(uint32_t nSessionId, uint64_t nToken)
{
  admin::Message message;
  message.mutable_access()->set_success(nToken);
  send(nSessionId, message);
}

void AdministratorPanel::sendLoginFailed(uint32_t nSessionId)
{
  admin::Message message;
  message.mutable_access()->set_fail(true);
  send(nSessionId, message);
}

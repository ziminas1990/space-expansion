#include "Scenarios.h"

#include <gtest/gtest.h>

namespace autotests
{

FunctionalTestFixture* Scenarios::m_pEnv = nullptr;

Scenarios::LoginScenario::LoginScenario(FunctionalTestFixture *pEnv)
  : BaseScenario(pEnv), lSendRequest(false), lExpectSuccess(false)
{}

Scenarios::LoginScenario& Scenarios::LoginScenario::sendLoginRequest(
    std::string const& sLogin, std::string const& sPassword)
{
  lSendRequest    = true;
  this->sLogin    = sLogin;
  this->sPassword = sPassword;
  return *this;
}

bool Scenarios::LoginScenario::run()
{
  if (lSendRequest) {
    if(!m_pEnv->m_pClientAccessPoint->sendLoginRequest(
         sLogin, sPassword, "127.0.0.1", m_pEnv->m_clientAddress.port()))
    {
      EXPECT_TRUE(false) << "Failed to send LoginRequest";
      return false;
    }
  }

  if (lExpectSuccess) {
    uint16_t nPort = 0;
    if (!m_pEnv->m_pClientAccessPoint->waitLoginSuccess(nPort) ||
        nPort == 0) {
      EXPECT_TRUE(false) << "No LoginSuccess response, or Port is 0";
      return false;
    }
    m_pEnv->m_serverAddress.port(nPort);
    m_pEnv->m_pClientUdpSocket->setServerAddress(m_pEnv->m_serverAddress);
  } else {
    if (!m_pEnv->m_pClientAccessPoint->waitLoginFailed()) {
      EXPECT_TRUE(false) << "No LoginFailed response";
    }
  }
  return true;
}


} // namespace autotes

#include "Scenarios.h"

#include <gtest/gtest.h>

namespace autotests
{

FunctionalTestFixture* Scenarios::m_pEnv = nullptr;

//========================================================================================
// Scenarios::LoginScenario
//========================================================================================

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

void Scenarios::LoginScenario::execute()
{
  if (lSendRequest) {
    ASSERT_TRUE(
          m_pEnv->m_pClientAccessPoint->sendLoginRequest(
            sLogin, sPassword, "127.0.0.1", m_pEnv->m_clientAddress.port()))
        << "Failed to send LoginRequest";
  }

  if (lExpectSuccess) {
    uint16_t nPort = 0;
    ASSERT_TRUE(m_pEnv->m_pClientAccessPoint->waitLoginSuccess(nPort))
        << "No LoginSuccess response, or Port is 0";
    ASSERT_NE(0, nPort) << "Port is 0";
    m_pEnv->m_serverAddress.port(nPort);
    m_pEnv->m_pClientUdpSocket->setServerAddress(m_pEnv->m_serverAddress);
  } else {
    ASSERT_TRUE(m_pEnv->m_pClientAccessPoint->waitLoginFailed())
        << "No LoginFailed response";
  }
}

//========================================================================================
// Scenarios::CheckAttachedModeuls
//========================================================================================

Scenarios::CheckAttachedModulesScenario::CheckAttachedModulesScenario(
    ClientCommutatorPtr pCommutator, FunctionalTestFixture *pEnv)
  : BaseScenario (pEnv), pCommutator(pCommutator)
{

}

Scenarios::CheckAttachedModulesScenario&
Scenarios::CheckAttachedModulesScenario::hasModule(
    std::string const& sType, size_t nCount)
{
  expectedModules[sType] = nCount;
  return *this;
}

void Scenarios::CheckAttachedModulesScenario::execute()
{
  uint32_t nExpectedTotal = 0;
  for (auto const& typeAndCount : expectedModules)
    nExpectedTotal += typeAndCount.second;

  ModulesList attachedModules;
  ASSERT_TRUE(pCommutator->getAttachedModulesList(nExpectedTotal, attachedModules))
      << "Can't get attached modules list from commutator!";

  for (ModuleInfo const& module : attachedModules)
  {
    ASSERT_NE(expectedModules.end(), expectedModules.find(module.sModuleType))
        << "Unexpected module with type " << module.sModuleType;
    if (0 == --expectedModules[module.sModuleType])
      expectedModules.erase(module.sModuleType);
  }
  ASSERT_TRUE(expectedModules.empty());
}

//========================================================================================
// Scenarios fabric methods
//========================================================================================

Scenarios::CheckAttachedModulesScenario Scenarios::CheckAttachedModules(
    ClientCommutatorPtr pClientCommutator)
{
  return CheckAttachedModulesScenario(pClientCommutator, m_pEnv);
}


} // namespace autotes

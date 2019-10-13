#include "Scenarios.h"

#include <assert.h>
#include <Utils/WaitingFor.h>
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
          m_pEnv->m_pAccessPanel->sendLoginRequest(
            sLogin, sPassword, "127.0.0.1", m_pEnv->m_clientAddress.port()))
        << "Failed to send LoginRequest";
  }

  if (lExpectSuccess) {
    uint16_t nPort = 0;
    ASSERT_TRUE(m_pEnv->m_pAccessPanel->waitLoginSuccess(nPort))
        << "No LoginSuccess response";
    ASSERT_NE(0, nPort) << "Port is 0";
    m_pEnv->m_serverAddress.port(nPort);
    m_pEnv->m_pSocket->setServerAddress(m_pEnv->m_serverAddress);
  } else {
    ASSERT_TRUE(m_pEnv->m_pAccessPanel->waitLoginFailed())
        << "No LoginFailed response";
  }
}

//========================================================================================
// Scenarios::CheckAttachedModeuls
//========================================================================================

Scenarios::CheckAttachedModulesScenario::CheckAttachedModulesScenario(
    client::ClientCommutatorPtr pCommutator, FunctionalTestFixture *pEnv)
  : BaseScenario (pEnv), pCommutator(pCommutator)
{}

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

  client::ModulesList attachedModules;
  ASSERT_TRUE(pCommutator->getAttachedModulesList(nExpectedTotal, attachedModules))
      << "Can't get attached modules list from commutator!";

  for (client::ModuleInfo const& module : attachedModules)
  {
    ASSERT_NE(expectedModules.end(), expectedModules.find(module.sModuleType))
        << "Unexpected module with type " << module.sModuleType;
    if (0 == --expectedModules[module.sModuleType])
      expectedModules.erase(module.sModuleType);
  }
  ASSERT_TRUE(expectedModules.empty());
}

//========================================================================================
// Scenarios::RunProceduresScenario
//========================================================================================

Scenarios::RunProceduresScenario::RunProceduresScenario(FunctionalTestFixture* pEnv)
  : BaseScenario(pEnv)
{
  m_batches.emplace_back();
}

Scenarios::RunProceduresScenario&
Scenarios::RunProceduresScenario::add(client::AbstractProcedurePtr pProcedure)
{
  assert(pProcedure != nullptr);
  m_batches.back().m_procedures.push_back(std::move(pProcedure));
  return *this;
}

Scenarios::RunProceduresScenario&
Scenarios::RunProceduresScenario::wait(uint32_t m_nIntervalMs, uint16_t nTimeoutMs)
{
  ProceduresBatch& batch = m_batches.back();
  batch.m_nIntervalMs    = m_nIntervalMs;
  batch.m_nTimeoutMs     = nTimeoutMs;
  m_batches.emplace_back();
  return *this;
}

void Scenarios::RunProceduresScenario::execute()
{
  auto fPredicate = [this]() { return isFrontBatchComplete(); };
  auto fProceeder = [this]() { proceedFrontBatch(); };

  while(!m_batches.empty()) {
    ProceduresBatch& batch = m_batches.front();
    if (!batch.m_procedures.empty()) {
      ASSERT_TRUE(utils::waitFor(fPredicate, fProceeder, batch.m_nTimeoutMs));
    }
    m_batches.pop_front();
  }
}

bool Scenarios::RunProceduresScenario::isFrontBatchComplete() const
{
  ProceduresBatch const& batch = m_batches.front();
  for (client::AbstractProcedurePtr const& pProcedure : batch.m_procedures) {
    if (!pProcedure->isComplete())
      return false;
  }
  return true;
}

void Scenarios::RunProceduresScenario::proceedFrontBatch()
{
  ProceduresBatch& batch = m_batches.front();
  m_pEnv->proceedEnviroment(batch.m_nIntervalMs);
  for (client::AbstractProcedurePtr& pProcedure : batch.m_procedures) {
    if (!pProcedure->isComplete())
      pProcedure->proceed(batch.m_nIntervalMs * 1000);
  }
}


//========================================================================================
// Scenarios fabric methods
//========================================================================================

Scenarios::CheckAttachedModulesScenario Scenarios::CheckAttachedModules(
    client::ClientCommutatorPtr pClientCommutator)
{
  return CheckAttachedModulesScenario(pClientCommutator, m_pEnv);
}

} // namespace autotes

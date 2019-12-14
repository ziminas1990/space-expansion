#pragma once

#include <list>
#include "FunctionalTestFixture.h"
#include <Protocol.pb.h>
#include <Autotests/ClientSDK/Procedures/AbstractProcedure.h>

namespace autotests
{

class Scenarios
{
  friend class FunctionalTestFixture;
public:

  class BaseScenario
  {
  public:
    BaseScenario(FunctionalTestFixture* pEnv) : m_pEnv(pEnv) {}
    virtual ~BaseScenario() = default;

    // HACK: allows to run scenario in ASSERT_TRUE or EXPECT_TRUE without calling run()
    operator bool() const { return const_cast<BaseScenario*>(this)->run(); }

    bool run() {
      execute();
      return !::testing::Test::HasFailure();
    }

  protected:
    virtual void execute() = 0;

  protected:
    FunctionalTestFixture* m_pEnv;
  };


  class LoginScenario : public BaseScenario
  {
  public:
    LoginScenario(FunctionalTestFixture* pEnv);

    LoginScenario& sendLoginRequest(std::string const& sLogin,
                                    std::string const& sPassword);

    LoginScenario& expectSuccess() { lExpectSuccess = true;  return *this; }
    LoginScenario& expectFailed()  { lExpectSuccess = false; return *this; }

  protected:
    void execute() override;

  private:
    bool lSendRequest = false;
    std::string sLogin;
    std::string sPassword;
    std::string sIP;
    uint16_t    nPort;

    bool lExpectSuccess = false;
  };


  class CheckAttachedModulesScenario : public BaseScenario
  {
  public:
    CheckAttachedModulesScenario(client::ClientCommutatorPtr pCommutator,
                                 FunctionalTestFixture* pEnv);

    CheckAttachedModulesScenario&
    hasModule(std::string const& sType, std::string const& name);

  protected:
    void execute() override;

  private:
    client::ClientCommutatorPtr pCommutator;
    std::set<std::pair<std::string, std::string>> expectedModules;
  };


  class RunProceduresScenario : public BaseScenario
  {
    using ProceduresVector = std::vector<client::AbstractProcedurePtr>;
  public:
    RunProceduresScenario(FunctionalTestFixture *pEnv);

    RunProceduresScenario& add(client::AbstractProcedurePtr pProcedure);
    RunProceduresScenario& wait(uint32_t nIntervalMs = 50, uint16_t nTimeoutMs = 500,
                                uint32_t nTickUs = 5000);

  protected:
    void execute() override;

  private:
    bool isFrontBatchComplete() const;
    void proceedFrontBatch();

  private:
    struct ProceduresBatch {
      ProceduresVector m_procedures;
      uint32_t m_nIntervalMs = 50;
      uint16_t m_nTimeoutMs  = 500;
      uint32_t m_nTickUs     = 5000;
    };

    struct std::list<ProceduresBatch> m_batches;
  };


  // =====================================================================================
  // Easy creation of scenarios objects:
  static LoginScenario Login() { return LoginScenario(m_pEnv); }

  static CheckAttachedModulesScenario
  CheckAttachedModules(client::ClientCommutatorPtr pClientCommutator);

  static RunProceduresScenario RunProcedures() { return RunProceduresScenario(m_pEnv); }

private:
  static FunctionalTestFixture* m_pEnv;
};

} // namespace autotests

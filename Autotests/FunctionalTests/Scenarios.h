#pragma once

#include "FunctionalTestFixture.h"
#include <Protocol.pb.h>

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
    hasModule(std::string const& sType, size_t nCount = 1);

  protected:
    void execute() override;

  private:
    client::ClientCommutatorPtr pCommutator;
    std::map<std::string, size_t> expectedModules;
  };

  // =====================================================================================
  // Easy creation of scenarios objects:
  static LoginScenario Login() { return LoginScenario(m_pEnv); }

  static CheckAttachedModulesScenario
  CheckAttachedModules(client::ClientCommutatorPtr pClientCommutator);

private:
  static FunctionalTestFixture* m_pEnv;
};

} // namespace autotests

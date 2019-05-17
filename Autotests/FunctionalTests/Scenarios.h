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

    virtual bool run() = 0;
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

    bool run() override;

  private:
    bool lSendRequest = false;
    std::string sLogin;
    std::string sPassword;
    std::string sIP;
    uint16_t    nPort;

    bool lExpectSuccess = false;
  };

  static LoginScenario Login() { return LoginScenario(m_pEnv); }

private:
  static FunctionalTestFixture* m_pEnv;
};

} // namespace autotests

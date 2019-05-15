#include "FunctionalTestFixture.h"

namespace autotests
{

using LoginFunctionalTests = FunctionalTestFixture;

TEST_F(LoginFunctionalTests, SuccessCase)
{
  ASSERT_TRUE(m_pClientAccessPoint->sendLoginRequest(
                "admin", "admin", "127.0.0.1", m_clientAddress.port()));

  uint16_t nPort = 0;
  ASSERT_TRUE(m_pClientAccessPoint->waitLoginSuccess(nPort));
  EXPECT_NE(0, nPort);
}

TEST_F(LoginFunctionalTests, LoginFailed)
{
  ASSERT_TRUE(m_pClientAccessPoint->sendLoginRequest(
                "admin", "sdfsdf", "127.0.0.1", m_clientAddress.port()));
  ASSERT_TRUE(m_pClientAccessPoint->waitLoginFailed());
}

} // namespace autotests

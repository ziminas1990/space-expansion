#include "FunctionalTestFixture.h"

#include "Scenarios.h"

namespace autotests
{

using LoginFunctionalTests = FunctionalTestFixture;

TEST_F(LoginFunctionalTests, SuccessCase)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("admin", "admin")
        .expectSuccess()
        .run());
}

TEST_F(LoginFunctionalTests, LoginFailed)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("dsdf", "sdfsdf")
        .expectFailed()
        .run());
}

} // namespace autotests

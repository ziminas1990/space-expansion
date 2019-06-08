#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class LoginFunctionalTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::stringstream ss;
    ss    <<              "Blueprints:"
          << std::endl << "  CommandCenter:"
          << std::endl << "    weight : 4000000"
          << std::endl << "    radius : 800"
          << std::endl << "Players:"
          << std::endl << "  admin:"
          << std::endl << "    password: admin"
          << std::endl << "    ships:"
          << std::endl << "      CommandCenter:"
          << std::endl << "        position: { x: 0, y: 0}"
          << std::endl << "        velocity: { x: 0, y: 0}";
    state = YAML::Load(ss.str());
    return true;
  }
};

TEST_F(LoginFunctionalTests, SuccessCase)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("admin", "admin")
        .expectSuccess());
}

TEST_F(LoginFunctionalTests, LoginFailed_IncorrectLogin)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("dsdf", "admin")
        .expectFailed());
}

TEST_F(LoginFunctionalTests, LoginFailed_IncorrectPassword)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("admin", "sdfsdf")
        .expectFailed());
}

} // namespace autotests

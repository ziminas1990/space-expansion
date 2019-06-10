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
    std::string data[] = {
      "Blueprints:",
      "  CommandCenter:",
      "    weight : 4000000",
      "    radius : 800",
      "Players:",
      "  admin:",
      "    password: admin",
      "    ships:",
      "      CommandCenter:",
      "        position: { x: 0, y: 0}",
      "        velocity: { x: 0, y: 0}"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
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

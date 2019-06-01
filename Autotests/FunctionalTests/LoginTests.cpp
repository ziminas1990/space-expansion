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
    ss << "Blueprints:"           << std::endl <<
          "  CommandCenter:"      << std::endl <<
          "    weight : 4000000"  << std::endl <<
          "  Corvet:"             << std::endl <<
          "    weight : 500000"   << std::endl <<
          "  Miner:"              << std::endl <<
          "    weight : 300000"   << std::endl <<
          "  Zond:"               << std::endl <<
          "    weight : 10000";
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

TEST_F(LoginFunctionalTests, LoginFailed)
{
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("dsdf", "sdfsdf")
        .expectFailed());
}

} // namespace autotests

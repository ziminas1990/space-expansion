#include "FunctionalTestFixture.h"

#include <Autotests/ClientSDK/Modules/ClientAdministratorPanel.h>
#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class AdministrationPanelTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    // Some minimal configuraton
    std::string data[] = {
      "Blueprints:",
      "  Ships:",
      "    CommandCenter:",
      "      weight : 4000000",
      "      radius : 800",
      "      expenses:",
      "        labor: 1000",
      "Players:",
      "  beggar:",
      "    password: alms",
      "    ships:",
      "      CommandCenter/Pentagon:",
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

TEST_F(AdministrationPanelTests, Login)
{
  uint64_t nToken = 0;

  ASSERT_TRUE(m_pAdminPanel->login("hacker", "fjdvhuerbn", nToken) ==
              client::AdministratorPanel::eLoginFailed);

  ASSERT_TRUE(m_pAdminPanel->login("god", "god", nToken) ==
              client::AdministratorPanel::eSuccess);
}

} // namespace autotests

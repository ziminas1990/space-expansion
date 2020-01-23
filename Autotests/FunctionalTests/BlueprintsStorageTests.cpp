#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientBlueprintStorage.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

class BlueprintStorageTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Modules:",
      "    CelestialScanner:",
      "      tiny-scanner:",
      "        max_scanning_radius_km: 200000",
      "        processing_time_us:     10",
      "      civilian-scanner:",
      "        max_scanning_radius_km: 2000000",
      "        processing_time_us:     20",
      "      huge-scanner:",
      "        max_scanning_radius_km: 20000000",
      "        processing_time_us:     40",
      "    AsteroidScanner:",
      "      tiny-scanner:",
      "        max_scanning_distance:  10000",
      "        scanning_time_ms:       100",
      "      civilian-scanner:",
      "        max_scanning_distance:  100000",
      "        scanning_time_ms:       200",
      "      huge-scanner:",
      "        max_scanning_distance:  100000",
      "        scanning_time_ms:       400",
      "    Engine:",
      "      ancient-nordic-engine:",
      "        maxThrust: 5000",
      "      civilian-engine:",
      "        maxThrust: 500000",
      "      titanic-engine:",
      "        maxThrust: 50000000",
      "    ResourceContainer:",
      "      toy-cargo:",
      "        volume: 1",
      "      civilian-cargo:",
      "        volume: 10",
      "      huge-cargo:",
      "        volume: 100",
      "      titanic-cargo:",
      "        volume: 1000",
      "  Ships:",
      "    Tiny-Scout:",
      "      radius: 10",
      "      weight: 10000",
      "      modules:",
      "        celestial-scanner: CelestialScanner/tiny-scanner",
      "        asteroid-scanner:  AsteroidScanner/tiny-scanner",
      "        engine:            Engine/ancient-nordic-engine",
      "    Civilian-Scout:",
      "      radius: 50",
      "      weight: 1000000",
      "      modules:",
      "        celestial-scanner: CelestialScanner/tiny-scanner",
      "        asteroid-scanner:  AsteroidScanner/tiny-scanner",
      "        engine:            Engine/ancient-nordic-engine",
      "    Titanic-Scout:",
      "      radius: 250",
      "      weight: 100000000",
      "      modules:",
      "        celestial-scanner: CelestialScanner/huge-scanner",
      "        asteroid-scanner:  AsteroidScanner/huge-scanner",
      "        engine:            Engine/titanic-engine",
      "Players:",
      "  James:",
      "    password: Bond"
    };
    std::stringstream ss;
    for (std::string const& line : data)
      ss << line << "\n";
    state = YAML::Load(ss.str());
    return true;
  }
};

TEST_F(BlueprintStorageTests, GetAllModules)
{
  animateWorld();
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("James", "Bond")
        .expectSuccess());

  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*m_pRootCommutator, storage));

  std::set<client::BlueprintName> expected = {
    client::BlueprintName("CelestialScanner/tiny-scanner"),
    client::BlueprintName("CelestialScanner/civilian-scanner"),
    client::BlueprintName("CelestialScanner/huge-scanner"),
    client::BlueprintName("AsteroidScanner/tiny-scanner"),
    client::BlueprintName("AsteroidScanner/civilian-scanner"),
    client::BlueprintName("AsteroidScanner/huge-scanner"),
    client::BlueprintName("Engine/ancient-nordic-engine"),
    client::BlueprintName("Engine/civilian-engine"),
    client::BlueprintName("Engine/titanic-engine"),
    client::BlueprintName("ResourceContainer/toy-cargo"),
    client::BlueprintName("ResourceContainer/civilian-cargo"),
    client::BlueprintName("ResourceContainer/huge-cargo"),
    client::BlueprintName("ResourceContainer/titanic-cargo")
  };

  std::vector<client::BlueprintName> modulesBlueprintsNames;
  ASSERT_TRUE(storage.getModulesBlueprintsNames("", modulesBlueprintsNames));

  for (client::BlueprintName const& name : modulesBlueprintsNames) {
    auto I = expected.find(name);
    ASSERT_NE(expected.end(), I) << " on " << name.getClass() << "/" << name.getType();
    expected.erase(I);
  }
  ASSERT_TRUE(expected.empty());
}

TEST_F(BlueprintStorageTests, GetSomeModules)
{
  animateWorld();
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("James", "Bond")
        .expectSuccess());

  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*m_pRootCommutator, storage));

  std::set<client::BlueprintName> expected = {
    client::BlueprintName("AsteroidScanner/tiny-scanner"),
    client::BlueprintName("AsteroidScanner/civilian-scanner"),
    client::BlueprintName("AsteroidScanner/huge-scanner"),
  };

  std::vector<client::BlueprintName> modulesBlueprintsNames;
  ASSERT_TRUE(storage.getModulesBlueprintsNames("AsteroidScan", modulesBlueprintsNames));

  for (client::BlueprintName const& name : modulesBlueprintsNames) {
    auto I = expected.find(name);
    ASSERT_NE(expected.end(), I) << " on " << name.getClass() << "/" << name.getType();
    expected.erase(I);
  }
  ASSERT_TRUE(expected.empty());
}

} // namespace autotests

#include "FunctionalTestFixture.h"

#include "Scenarios.h"

#include <Autotests/ClientSDK/Modules/ClientBlueprintStorage.h>
#include <Autotests/ClientSDK/Procedures/FindModule.h>

#include <yaml-cpp/yaml.h>
#include <sstream>

namespace autotests
{

static bool hasResource(world::Resources const& resources,
                        world::ResourceItem const& expected)
{
  for (world::ResourceItem const& resource : resources) {
    if (resource == expected)
      return true;
  }
  return false;
}

class BlueprintStorageTests : public FunctionalTestFixture
{
protected:
  // overrides from FunctionalTestFixture interface
  bool initialWorldState(YAML::Node& state) {
    std::string data[] = {
      "Blueprints:",
      "  Modules:",
      "    AsteroidScanner:",
      "      tiny-scanner:",
      "        max_scanning_distance:  10000",
      "        scanning_time_ms:       100",
      "        expenses:",
      "          labor: 10",
      "      civilian-scanner:",
      "        max_scanning_distance:  100000",
      "        scanning_time_ms:       200",
      "        expenses:",
      "          labor: 100",
      "      huge-scanner:",
      "        max_scanning_distance:  100000",
      "        scanning_time_ms:       400",
      "        expenses:",
      "          labor:     500",
      "          metals:    2000",
      "          silicates: 1000",
      "          ice:       500",
      "    Engine:",
      "      ancient-nordic-engine:",
      "        max_thrust: 5000",
      "        expenses:",
      "          labor: 10",
      "      civilian-engine:",
      "        max_thrust: 500000",
      "        expenses:",
      "          labor: 100",
      "      titanic-engine:",
      "        max_thrust: 50000000",
      "        expenses:",
      "          labor:     1000",
      "          metals:    5000",
      "          silicates: 2000",
      "          ice:       1000",
      "    ResourceContainer:",
      "      toy-cargo:",
      "        volume: 1",
      "        expenses:",
      "          labor: 1",
      "      civilian-cargo:",
      "        volume: 10",
      "        expenses:",
      "          labor: 10",
      "      huge-cargo:",
      "        volume: 100",
      "        expenses:",
      "          labor: 100",
      "      titanic-cargo:",
      "        volume: 1000",
      "        expenses:",
      "          labor: 1000",
      "  Ships:",
      "    Tiny-Scout:",
      "      radius: 10",
      "      weight: 10000",
      "      modules:",
      "        asteroid-scanner:  AsteroidScanner/tiny-scanner",
      "        engine:            Engine/ancient-nordic-engine",
      "      expenses:",
      "        labor: 100",
      "    Civilian-Scout:",
      "      radius: 50",
      "      weight: 1000000",
      "      modules:",
      "        asteroid-scanner:  AsteroidScanner/tiny-scanner",
      "        engine:            Engine/ancient-nordic-engine",
      "      expenses:",
      "        labor: 1000",
      "    Titanic-Scout:",
      "      radius: 250",
      "      weight: 100000000",
      "      modules:",
      "        asteroid-scanner:  AsteroidScanner/huge-scanner",
      "        engine:            Engine/titanic-engine",
      "      expenses:",
      "          labor:     10000",
      "          metals:    40000",
      "          silicates: 20000",
      "          ice:       10000",
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
  resumeTime();
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("James", "Bond")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*pCommutator, storage));

  std::set<client::BlueprintName> expected = {
    client::BlueprintName("AsteroidScanner/tiny-scanner"),
    client::BlueprintName("AsteroidScanner/civilian-scanner"),
    client::BlueprintName("AsteroidScanner/huge-scanner"),
    client::BlueprintName("Engine/ancient-nordic-engine"),
    client::BlueprintName("Engine/civilian-engine"),
    client::BlueprintName("Engine/titanic-engine"),
    client::BlueprintName("ResourceContainer/toy-cargo"),
    client::BlueprintName("ResourceContainer/civilian-cargo"),
    client::BlueprintName("ResourceContainer/huge-cargo"),
    client::BlueprintName("ResourceContainer/titanic-cargo"),
    client::BlueprintName("Ship/Tiny-Scout"),
    client::BlueprintName("Ship/Civilian-Scout"),
    client::BlueprintName("Ship/Titanic-Scout")
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
  resumeTime();
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("James", "Bond")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*pCommutator, storage));

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

TEST_F(BlueprintStorageTests, GetNonExistingModuleBlueprint)
{
  resumeTime();
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("James", "Bond")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*pCommutator, storage));

  client::Blueprint blueprint;
  ASSERT_EQ(client::BlueprintsStorage::eBlueprintNotFound,
            storage.getBlueprint(client::BlueprintName("abyrvalg"), blueprint));

  ASSERT_EQ(client::BlueprintsStorage::eBlueprintNotFound,
            storage.getBlueprint(client::BlueprintName("abyrvalg/sharikov"), blueprint));
}

TEST_F(BlueprintStorageTests, GetSomeModulesBlueprints)
{
  resumeTime();
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("James", "Bond")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*pCommutator, storage));

  {
    client::Blueprint blueprint;
    ASSERT_EQ(client::BlueprintsStorage::eSuccess,
              storage.getBlueprint(
                client::BlueprintName("AsteroidScanner/tiny-scanner"),
                blueprint));
    EXPECT_EQ("AsteroidScanner/tiny-scanner", blueprint.m_sName);
    EXPECT_EQ("10000", blueprint.m_properties["max_scanning_distance"]->sValue);
    EXPECT_EQ("100",   blueprint.m_properties["scanning_time_ms"]->sValue);
  }

  {
    client::Blueprint blueprint;
    ASSERT_EQ(client::BlueprintsStorage::eSuccess,
              storage.getBlueprint(
                client::BlueprintName("Engine/civilian-engine"),
                blueprint));
    EXPECT_EQ("Engine/civilian-engine", blueprint.m_sName);
    EXPECT_EQ("500000", blueprint.m_properties["max_thrust"]->sValue);
  }

  {
    client::Blueprint blueprint;
    ASSERT_EQ(client::BlueprintsStorage::eSuccess,
              storage.getBlueprint(
                client::BlueprintName("ResourceContainer/titanic-cargo"),
                blueprint));
    EXPECT_EQ("ResourceContainer/titanic-cargo", blueprint.m_sName);
    EXPECT_EQ("1000", blueprint.m_properties["volume"]->sValue);
  }
}

TEST_F(BlueprintStorageTests, GetShipBlueprints)
{
  resumeTime();
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("James", "Bond")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*pCommutator, storage));

  {
    client::Blueprint blueprint;
    ASSERT_EQ(client::BlueprintsStorage::eSuccess,
              storage.getBlueprint(
                client::BlueprintName("Ship/Titanic-Scout"),
                blueprint));
    EXPECT_EQ("Ship/Titanic-Scout", blueprint.m_sName);
    EXPECT_EQ("250",       blueprint.m_properties["radius"]->sValue);
    EXPECT_EQ("100000000", blueprint.m_properties["weight"]->sValue);

    client::PropertyUniqPtr const& pShipModules = blueprint.m_properties["modules"];
    EXPECT_EQ("AsteroidScanner/huge-scanner",
              pShipModules->nested["asteroid-scanner"]->sValue);
    EXPECT_EQ("Engine/titanic-engine",
              pShipModules->nested["engine"]->sValue);
    EXPECT_TRUE(hasResource(blueprint.m_expenses, world::ResourceItem::metals(47000)));
    EXPECT_TRUE(hasResource(blueprint.m_expenses,
                            world::ResourceItem::silicates(23000)));
    EXPECT_TRUE(hasResource(blueprint.m_expenses, world::ResourceItem::ice(11500)));
    EXPECT_TRUE(hasResource(blueprint.m_expenses, world::ResourceItem::labor(11500)));
  }
}

TEST_F(BlueprintStorageTests, GetAllModulesBlueprints)
{
  resumeTime();
  ASSERT_TRUE(
        Scenarios::Login()
        .sendLoginRequest("James", "Bond")
        .expectSuccess());
  client::ClientCommutatorPtr pCommutator = openCommutatorSession();
  ASSERT_TRUE(pCommutator);

  client::BlueprintsStorage storage;
  ASSERT_TRUE(client::FindBlueprintStorage(*pCommutator, storage));

  std::vector<client::BlueprintName> modulesBlueprintsNames;
  ASSERT_TRUE(storage.getModulesBlueprintsNames("", modulesBlueprintsNames));

  for (client::BlueprintName const& name : modulesBlueprintsNames) {
    client::Blueprint blueprint;
    ASSERT_EQ(client::BlueprintsStorage::eSuccess,
              storage.getBlueprint(name, blueprint));
    EXPECT_EQ(name.getFullName(), blueprint.m_sName);
  }
}

} // namespace autotests

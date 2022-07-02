#include "ShipBlueprint.h"

#include <yaml-cpp/yaml.h>
#include <Modules/Ship/Ship.h>
#include <Blueprints/BlueprintsLibrary.h>
#include <World/Player.h>
#include <Utils/YamlReader.h>
#include <Utils/YamlDumper.h>
#include <Utils/StringUtils.h>

namespace blueprints {

ShipBlueprint::ShipBlueprint(std::string sShipProjectName)
  : m_sType(std::move(sShipProjectName))
{}

modules::BaseModulePtr ShipBlueprint::build(
    std::string sName, world::PlayerWeakPtr pOwner) const
{
  BlueprintsLibrary& library = pOwner.lock()->getBlueprints();
  return build(std::move(sName), pOwner, library);
}

modules::ShipPtr ShipBlueprint::build(
    std::string sName,
    world::PlayerWeakPtr pOwner,
    BlueprintsLibrary const& customLibrary) const
{
  modules::ShipPtr pShip =
      std::make_shared<modules::Ship>(
        m_sType, std::move(sName), pOwner, m_weight, m_radius);

  for (auto const& kv : m_modules)
  {
    BaseBlueprintPtr pBlueprint = customLibrary.getBlueprint(kv.second);
    assert(pBlueprint);
    if (!pBlueprint) {
      return modules::ShipPtr();
    }

    modules::BaseModulePtr pModule = pBlueprint->build(kv.first, pOwner);
    pShip->installModule(std::move(pModule));
  }
  return pShip;
}

bool ShipBlueprint::checkDependencies(BlueprintsLibrary const& library) const
{
  for (auto const& kv : m_modules) {
    // kv.second is BlueprintName object
    if (!library.hasBlueprint(kv.second))
      return false;
  }
  return true;
}

bool ShipBlueprint::exportTotalExpenses(BlueprintsLibrary const& library,
                                        world::ResourcesArray& total) const
{
  total = expenses(); // hull expenses
  for (auto const& kv : m_modules) {
    BlueprintName const& name             = kv.second;
    BaseBlueprintPtr     pModuleBlueprint = library.getBlueprint(name);
    if (!pModuleBlueprint)
      return false;
    pModuleBlueprint->expenses(total);
  }
  return true;
}

bool ShipBlueprint::load(YAML::Node const& data)
{
  if (!BaseBlueprint::load(data)) {
    return false;
  }

  if (!utils::YamlReader(data)
      .read("weight", m_weight)
      .read("radius", m_radius)) {
    assert(false);
    return false;
  }

  for (auto const& kv : data["modules"]) {
    std::string sModuleName = kv.first.as<std::string>();
    if (m_modules.find(sModuleName) != m_modules.end()) {
      // Duplicate detected
      assert(false);
      return false;
    }

    std::string const& sBlueprintName = kv.second.as<std::string>();
    std::string sModuleClass;
    std::string sModuleType;
    utils::StringUtils::split('/', sBlueprintName, sModuleClass, sModuleType);
    assert(!sModuleClass.empty() && !sModuleType.empty());
    m_modules.emplace(
          std::move(sModuleName),
          BlueprintName(std::move(sModuleClass), std::move(sModuleType)));
  }
  return true;
}

void ShipBlueprint::dump(YAML::Node& out) const
{
  BaseBlueprint::dump(out);

  YAML::Node modules;
  {
    utils::YamlDumper dumper(modules);
    for (auto const& kv : m_modules)
      // kv - { module_name, blueprint_name }
      dumper.add(kv.first.c_str(), kv.second.toString());
  }

  utils::YamlDumper dumper(out);
  dumper.add("weight", m_weight)
        .add("radius", m_radius)
        .add("modules", std::move(modules));
}

} // namespace modules

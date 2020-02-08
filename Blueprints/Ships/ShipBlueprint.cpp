#include "ShipBlueprint.h"

#include <yaml-cpp/yaml.h>
#include <Utils/YamlReader.h>
#include <Utils/StringUtils.h>
#include <Blueprints/Modules/EngineBlueprint.h>

namespace ships {

ShipBlueprint::ShipBlueprint(std::string sShipProjectName)
  : m_sType(std::move(sShipProjectName))
{}

modules::BaseModulePtr ShipBlueprint::build(
    std::string sName, const modules::BlueprintsLibrary &library) const
{
  ShipPtr pShip = std::make_shared<Ship>(m_sType, std::move(sName), m_weight, m_radius);
  for (auto const& kv : m_modules)
  {
    modules::BaseBlueprintPtr pBlueprint = library.getBlueprint(kv.second);
    assert(pBlueprint);
    if (!pBlueprint) {
      return ShipPtr();
    }

    modules::BaseModulePtr pModule = pBlueprint->build(kv.first, library);
    pShip->installModule(std::move(pModule));
  }
  return pShip;
}

bool ShipBlueprint::load(YAML::Node const& data)
{
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
          modules::BlueprintName(std::move(sModuleClass), std::move(sModuleType)));
  }
  return true;
}

void ShipBlueprint::dump(YAML::Node& out) const
{
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

} // namespace ships

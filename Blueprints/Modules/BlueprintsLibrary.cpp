#include "BlueprintsLibrary.h"

#include "BlueprintFactory.h"
#include <yaml-cpp/yaml.h>

namespace modules {

bool BlueprintsLibrary::loadBlueprints(YAML::Node const& modulesSection)
{
  for(auto const& modulesClassInfo : modulesSection) {
    std::string const& sModuleClass = modulesClassInfo.first.as<std::string>();

    for (auto const& moduleTypeInfo : modulesClassInfo.second) {
      std::string const& sModuleType = moduleTypeInfo.first.as<std::string>();

      ModuleBlueprintPtr pBlueprint =
          BlueprintsFactory::make(sModuleClass, moduleTypeInfo.second);
      assert(pBlueprint != nullptr);
      if (!pBlueprint) {
        return false;
      }

      m_blueprints[BlueprintName(sModuleClass, sModuleType)] = pBlueprint;
    }
  }
  return true;
}

ModuleBlueprintPtr BlueprintsLibrary::getBlueprint(BlueprintName const& name) const
{
  auto I = m_blueprints.find(name);
  return I != m_blueprints.end() ? I->second : ModuleBlueprintPtr();
}

} // namespace modules

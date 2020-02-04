#include "BlueprintsStorage.h"

#include <Utils/StringUtils.h>
#include <yaml-cpp/yaml.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::BlueprintsStorage);

namespace modules {

BlueprintsStorage::BlueprintsStorage()
  : BaseModule("BlueprintsLibrary", "Central")
{
  GlobalContainer<BlueprintsStorage>::registerSelf(this);
}

void BlueprintsStorage::attachToLibraries(
    BlueprintsLibrary            const* pModulesBlueprints,
    ships::ShipBlueprintsLibrary const* pShipsBlueprints)
{
  m_pModulesBlueprints = pModulesBlueprints;
  m_pShipsBlueprints   = pShipsBlueprints;
}

void BlueprintsStorage::handleBlueprintsStorageMessage(
    uint32_t nSessionId, spex::IBlueprintsLibrary const& message)
{
  switch (message.choice_case()) {
    case spex::IBlueprintsLibrary::kModulesListReq:
      onModulesListReq(nSessionId, message.modules_list_req());
      return;
    case spex::IBlueprintsLibrary::kModuleBlueprintReq:
      onModuleBlueprintReq(nSessionId, message.module_blueprint_req());
      return;
    default:
      assert("Unsupported message!" == nullptr);
  }
}

void BlueprintsStorage::onModulesListReq(
    uint32_t nSessionId, std::string const& sFilter) const
{
  std::vector<std::string> modulesNamesList;
  modulesNamesList.reserve(10);
  m_pModulesBlueprints->iterate(
        [&modulesNamesList, &sFilter](modules::BlueprintName const& name) -> bool {
          std::string const& sFullName = name.toString();
          if (utils::StringUtils::startsWith(sFullName, sFilter))
            modulesNamesList.push_back(sFullName);
          return true;
        }
  );

  if (modulesNamesList.empty()) {
    spex::Message response;
    spex::IBlueprintsLibrary::NamesList* pBody =
        response.mutable_blueprints_library()->mutable_modules_list();
    pBody->set_left(0);
    sendToClient(nSessionId, response);
    return;
  }

  size_t nNamesPerMessage = 10;
  size_t nNamesLeft       = modulesNamesList.size();
  while (nNamesLeft) {
    spex::Message response;
    spex::IBlueprintsLibrary::NamesList* pBody =
        response.mutable_blueprints_library()->mutable_modules_list();
    for (size_t i = 0; i < nNamesPerMessage && nNamesLeft; ++i) {
       pBody->add_names(std::move(modulesNamesList[--nNamesLeft]));
    }
    pBody->set_left(static_cast<uint32_t>(nNamesLeft));
    sendToClient(nSessionId, response);
  }
}

void BlueprintsStorage::onModuleBlueprintReq(uint32_t nSessionId,
                                             std::string const& sName) const
{
  BlueprintName blueprintName = BlueprintName::make(sName);
  if (!blueprintName.isValid()) {
    sendModuleBlueprintFail(nSessionId, spex::IBlueprintsLibrary::BLUEPRINT_NOT_FOUND);
    return;
  }

  ModuleBlueprintPtr pBlueprint = m_pModulesBlueprints->getBlueprint(blueprintName);
  if (!pBlueprint) {
    sendModuleBlueprintFail(nSessionId, spex::IBlueprintsLibrary::BLUEPRINT_NOT_FOUND);
    return;
  }

  spex::Message response;
  spex::IBlueprintsLibrary::ModuleBlueprint* pBody =
      response.mutable_blueprints_library()->mutable_module_blueprint();
  pBody->set_name(sName);

  YAML::Node specification;
  pBlueprint->dump(specification);
  for (auto const& parameter: specification) {
    spex::IBlueprintsLibrary::Property* pProperty = pBody->add_properties();
    pProperty->set_name(parameter.first.as<std::string>());
    pProperty->set_value(parameter.second.as<std::string>());
  }

  sendToClient(nSessionId, response);
}

bool BlueprintsStorage::sendModuleBlueprintFail(
    uint32_t nSessionId, spex::IBlueprintsLibrary::Status error) const
{
  spex::Message response;
  response.mutable_blueprints_library()->set_module_blueprint_fail(error);
  return sendToClient(nSessionId, response);
}

} // namespace modules

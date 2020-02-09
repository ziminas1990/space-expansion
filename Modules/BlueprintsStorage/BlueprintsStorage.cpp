#include "BlueprintsStorage.h"

#include <Blueprints/BaseBlueprint.h>
#include <Utils/StringUtils.h>
#include <yaml-cpp/yaml.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::BlueprintsStorage);

namespace modules {

static void importProperties(YAML::Node const& from, spex::Property* to)
{
  for (auto const& parameter: from) {
    spex::Property* pProperty = to->add_nested();
    pProperty->set_name(parameter.first.as<std::string>());
    if (parameter.second.IsScalar()) {
      pProperty->set_value(parameter.second.as<std::string>());
    } else if (parameter.second.IsMap()) {
      importProperties(parameter.second, pProperty);
    } else {
      assert(false);
    }
  }
}

BlueprintsStorage::BlueprintsStorage()
  : BaseModule("BlueprintsLibrary", "Central")
{
  GlobalContainer<BlueprintsStorage>::registerSelf(this);
}

void BlueprintsStorage::attachToLibrary(BlueprintsLibrary const* pModulesBlueprints)
{
  m_pLibrary = pModulesBlueprints;
}

void BlueprintsStorage::handleBlueprintsStorageMessage(
    uint32_t nSessionId, spex::IBlueprintsLibrary const& message)
{
  switch (message.choice_case()) {
    case spex::IBlueprintsLibrary::kBlueprintsListReq:
      onModulesListReq(nSessionId, message.blueprints_list_req());
      return;
    case spex::IBlueprintsLibrary::kBlueprintReq:
      onModuleBlueprintReq(nSessionId, message.blueprint_req());
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
  m_pLibrary->iterate(
        [&modulesNamesList, &sFilter](modules::BlueprintName const& name) -> bool {
          std::string const& sFullName = name.toString();
          if (utils::StringUtils::startsWith(sFullName, sFilter))
            modulesNamesList.push_back(sFullName);
          return true;
        }
  );

  if (modulesNamesList.empty()) {
    spex::Message response;
    spex::NamesList* pBody =
        response.mutable_blueprints_library()->mutable_blueprints_list();
    pBody->set_left(0);
    sendToClient(nSessionId, response);
    return;
  }

  size_t nNamesPerMessage = 10;
  size_t nNamesLeft       = modulesNamesList.size();
  while (nNamesLeft) {
    spex::Message response;
    spex::NamesList* pBody =
        response.mutable_blueprints_library()->mutable_blueprints_list();
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

  BaseBlueprintPtr pBlueprint = m_pLibrary->getBlueprint(blueprintName);
  if (!pBlueprint) {
    sendModuleBlueprintFail(nSessionId, spex::IBlueprintsLibrary::BLUEPRINT_NOT_FOUND);
    return;
  }

  spex::Message response;
  spex::Blueprint* pBody = response.mutable_blueprints_library()->mutable_blueprint();
  pBody->set_name(sName);

  // Complex code: converting blueprint specification in that way:
  // Blueprint -> YAML -> Protobuf
  YAML::Node specification;
  pBlueprint->dump(specification);
  for (auto const& property: specification) {
    std::string propertyName = property.first.as<std::string>();
    if (propertyName == "expenses")
      // Expenses will be sent as separated "expenses" field
      continue;

    spex::Property* pItem = pBody->add_properties();
    pItem->set_name(std::move(propertyName));
    if (property.second.IsScalar()) {
      pItem->set_value(property.second.as<std::string>());
    } else if (property.second.IsMap()) {
      importProperties(property.second, pItem);
    } else {
      assert(false);
    }
  }

  sendToClient(nSessionId, response);
}

bool BlueprintsStorage::sendModuleBlueprintFail(
    uint32_t nSessionId, spex::IBlueprintsLibrary::Status error) const
{
  spex::Message response;
  response.mutable_blueprints_library()->set_blueprint_fail(error);
  return sendToClient(nSessionId, response);
}

} // namespace modules

#include "BlueprintsStorage.h"

#include <yaml-cpp/yaml.h>

#include <World/Player.h>
#include <Blueprints/BlueprintsLibrary.h>
#include <Blueprints/BaseBlueprint.h>
#include <Blueprints/Ships/ShipBlueprint.h>
#include <Utils/StringUtils.h>
#include <Utils/ItemsConverter.h>

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

BlueprintsStorage::BlueprintsStorage(world::PlayerWeakPtr pOwner)
  : BaseModule("BlueprintsLibrary", "Central", pOwner)
{
  GlobalContainer<BlueprintsStorage>::registerSelf(this);
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

  getLibrary().iterate(
        [&modulesNamesList, &sFilter](blueprints::BlueprintName const& name) -> bool {
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
  blueprints::BlueprintName blueprintName = blueprints::BlueprintName::make(sName);
  if (!blueprintName.isValid()) {
    sendModuleBlueprintFail(nSessionId, spex::IBlueprintsLibrary::BLUEPRINT_NOT_FOUND);
    return;
  }

  blueprints::BlueprintsLibrary const& library = getLibrary();
  blueprints::BaseBlueprintPtr pBlueprint = library.getBlueprint(blueprintName);
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

  world::ResourcesArray expenses;
  if (blueprintName.getModuleClass() == "Ship") {
    // Exception: for ship's blueprint we should return summ expenses for hull and all
    // ship's modules
    blueprints::ShipBlueprintPtr pShipBlueprint =
        std::dynamic_pointer_cast<blueprints::ShipBlueprint>(pBlueprint);
    assert(pShipBlueprint);
    pShipBlueprint->exportTotalExpenses(library, expenses);
  } else {
    pBlueprint->expenses(expenses);
  }

  for (size_t i = 0; i < expenses.size(); ++i) {
    if (expenses[i] > 0) {
      world::Resource::Type type = static_cast<world::Resource::Type>(i);
      spex::ResourceItem* pItem = pBody->add_expenses();
      pItem->set_type(utils::convert(type));
      pItem->set_amount(expenses[i]);
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

blueprints::BlueprintsLibrary const& BlueprintsStorage::getLibrary() const
{
  const static blueprints::BlueprintsLibrary emptyLibrary;

  world::PlayerPtr pOwner = getOwner().lock();
  return pOwner ? pOwner->getBlueprints() : emptyLibrary;
}

} // namespace modules

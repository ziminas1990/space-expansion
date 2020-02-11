#include "ClientBlueprintStorage.h"

#include <Utils/StringUtils.h>
#include <Utils/ItemsConverter.h>

namespace autotests { namespace client {

static BlueprintsStorage::Status convert(spex::IBlueprintsLibrary::Status status) {
  switch(status) {
    case spex::IBlueprintsLibrary::SUCCESS:
      return BlueprintsStorage::eSuccess;
    case spex::IBlueprintsLibrary::INTERNAL_ERROR:
      return BlueprintsStorage::eInternalError;
    case spex::IBlueprintsLibrary::BLUEPRINT_NOT_FOUND:
      return BlueprintsStorage::eBlueprintNotFound;
    default: {
      // To avoid dummy warning
      assert(false);
      return BlueprintsStorage::eInternalError;
    }
  }
}

BlueprintName::BlueprintName(std::string const& sName)
{
  utils::StringUtils::split('/', sName, m_sModuleClass, m_sModuleType);
}

bool BlueprintsStorage::getModulesBlueprintsNames(
    std::string const& sFilter, std::vector<BlueprintName>& output)
{
  spex::Message request;
  request.mutable_blueprints_library()->mutable_blueprints_list_req()->assign(sFilter);
  if (!send(request)) {
    return false;
  }

  size_t nLeft = 0;
  do {
    spex::IBlueprintsLibrary response;
    if (!wait(response)) {
      return false;
    }
    if (response.choice_case() != spex::IBlueprintsLibrary::kBlueprintsList) {
      return false;
    }
    nLeft             = response.blueprints_list().left();
    auto const& names = response.blueprints_list().names();
    for (std::string const& name : names) {
      output.push_back(name);
    }
  } while(nLeft);

  return true;
}

BlueprintsStorage::Status BlueprintsStorage::getBlueprint(
    BlueprintName const& name, Blueprint &out)
{
  spex::Message request;
  request.mutable_blueprints_library()->mutable_blueprint_req()->assign(
        name.getFullName());
  if (!send(request)) {
    return eTransportError;
  }

  spex::IBlueprintsLibrary response;
  if (!wait(response)) {
    return eTransportError;
  }
  if (response.choice_case() == spex::IBlueprintsLibrary::kBlueprintFail) {
    return convert(response.blueprint_fail());
  }
  if (response.choice_case() != spex::IBlueprintsLibrary::kBlueprint) {
    return eUnexpectedMessage;
  }

  spex::Blueprint const& body = response.blueprint();
  out.m_sName = body.name();
  for (spex::Property const& property : body.properties()) {
    PropertyUniqPtr pItem = std::make_unique<Property>();
    pItem->sName = property.name();
    pItem->sValue = property.value();

    // Let's assume, that only 2 layer hierarchy is possible
    for (spex::Property const& nested: property.nested()) {
      PropertyUniqPtr pNestedItem = std::make_unique<Property>();
      pNestedItem->sName = nested.name();
      pNestedItem->sValue = nested.value();
      pItem->nested[pNestedItem->sName] = std::move(pNestedItem);
    }

    out.m_properties[pItem->sName] = std::move(pItem);
  }

  for (spex::ResourceItem const& resource : body.expenses()) {
    out.m_expenses.push_back(utils::convert(resource));
  }

  return eSuccess;
}

}}  // namespace autotests::client

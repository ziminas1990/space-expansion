#include "ClientBlueprintStorage.h"

#include <Utils/StringUtils.h>

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
  request.mutable_blueprints_library()->mutable_modules_list_req()->assign(sFilter);
  if (!send(request)) {
    return false;
  }

  size_t nLeft = 0;
  do {
    spex::IBlueprintsLibrary response;
    if (!wait(response)) {
      return false;
    }
    if (response.choice_case() != spex::IBlueprintsLibrary::kModulesList) {
      return false;
    }
    nLeft             = response.modules_list().left();
    auto const& names = response.modules_list().names();
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
  request.mutable_blueprints_library()->mutable_module_blueprint_req()->assign(
        name.getFullName());
  if (!send(request)) {
    return eTransportError;
  }

  spex::IBlueprintsLibrary response;
  if (!wait(response)) {
    return eTransportError;
  }
  if (response.choice_case() == spex::IBlueprintsLibrary::kModuleBlueprintFail) {
    return convert(response.module_blueprint_fail());
  }
  if (response.choice_case() != spex::IBlueprintsLibrary::kModuleBlueprint) {
    return eUnexpectedMessage;
  }

  spex::IBlueprintsLibrary::ModuleBlueprint const& body = response.module_blueprint();
  out.m_sName = body.name();
  for (spex::IBlueprintsLibrary::Property const& property : body.properties()) {
    out.m_properties[property.parameter()] = property.value();
  }

  return eSuccess;
}

}}  // namespace autotests::client

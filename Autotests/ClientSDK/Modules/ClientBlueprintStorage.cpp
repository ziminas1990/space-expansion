#include "ClientBlueprintStorage.h"

#include <Utils/StringUtils.h>

namespace autotests { namespace client {

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

}}  // namespace autotests::client

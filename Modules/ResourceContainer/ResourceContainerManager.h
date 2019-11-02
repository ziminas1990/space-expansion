#include "Modules/CommonModulesManager.h"
#include "ResourceContainer.h"

namespace modules {

using ResourceContainerManager =
  CommonModulesManager<ResourceContainer, Cooldown::eAsteroidScanner>;
using ResourceContainerManagerPtr = std::shared_ptr<ResourceContainerManager>;

} // namespace modules

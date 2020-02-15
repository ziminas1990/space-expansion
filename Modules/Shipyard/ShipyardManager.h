#include "Modules/CommonModulesManager.h"
#include "Shipyard.h"

namespace modules {

using ShipyardManager    = CommonModulesManager<Shipyard, Cooldown::eResourceContainer>;
using ShipyardManagerPtr = std::shared_ptr<ShipyardManager>;

} // namespace modules

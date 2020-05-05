#include "Modules/CommonModulesManager.h"
#include "Shipyard.h"

namespace modules {

using ShipyardManager    = CommonModulesManager<Shipyard, Cooldown::eShipYard>;
using ShipyardManagerPtr = std::shared_ptr<ShipyardManager>;

} // namespace modules

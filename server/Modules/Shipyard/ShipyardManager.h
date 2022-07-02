#include "Modules/CommonModulesManager.h"
#include "Shipyard.h"

namespace modules {

class ShipyardManager
: public CommonModulesManager<Shipyard, Cooldown::eShipYard>
{};

} // namespace modules

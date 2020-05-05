#pragma once

#include <Autotests/ClientSDK/ClientBaseModule.h>

namespace autotests { namespace client {

struct ShipyardSpecification {
  double m_nLaborPerSec;
};

class Shipyard : public ClientBaseModule
{
public:

  enum Status {
    eSuccess,
    eInternalError,
    eBuildStarted,
    eBuildFreezed,
    eBuildFailed,
    eBuildCanceled,
    eBlueprintNotFound,
    eShipyardIsBusy,

    // Errors, detected on client side:
    eTransportError,
    eUnexpectedMessage,
    eTimeoutError,
    eStatusError
  };
  
  bool getSpecification(ShipyardSpecification& spec);
  Status startBuilding(std::string const& sBlueprint, std::string const& sShipName);
  Status cancelBuild();

  Status waitingWhileBuilding(uint32_t *pSlotId = nullptr,
                              std::string *pShipName = nullptr);
    // Receives the 'building_status' message, but ignores them.  If the
    // 'building_complete' message is received, thie function will write ship's slot and
    // name to the specified 'pSlotId' and 'pShipsName' arguments and return eSuccess. If
    // any 'build_statis' message is received, function returns received status. Otherwise
    // it returns the 'eUnexpectedMessage'
};

}}  // namespace autotests::client

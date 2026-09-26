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
    eCargoNotFound,
    eBuildStarted,
    eBuildInProgress,
    eBuildComplete,
    eBuildFrozen,
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

  struct BuildStarted {
    std::string blueprintName;
    std::string shipName;
  };

  bool getSpecification(ShipyardSpecification& spec);
  Status bindToCargo(std::string const& container);
  Status startBuilding(std::string const& sBlueprint, std::string const& sShipName);
  Status cancelBuild();

  // Subscribe to builds on this session. Waits for monitoring_ack.
  bool startMonitoring();
  bool waitBuildStarted(BuildStarted& started, uint16_t nTimeout = 500);
  bool waitBuildingReport(Status& status, double& progress, uint16_t nTimeout = 500);

  Status waitingWhileBuilding(double *progress,
                              uint32_t *pSlotId = nullptr,
                              std::string *pShipName = nullptr);
    // Receives the 'building_status' message, but ignores them.  If the
    // 'building_complete' message is received, thie function will write ship's slot and
    // name to the specified 'pSlotId' and 'pShipsName' arguments and return eSuccess. If
    // any 'build_statis' message is received, function returns received status. Otherwise
    // it returns the 'eUnexpectedMessage'
};

}}  // namespace autotests::client

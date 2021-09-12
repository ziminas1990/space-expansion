#include "ClientShipyard.h"

namespace autotests { namespace client {

Shipyard::Status convert(spex::IShipyard::Status eStatus)
{
  switch (eStatus) {
    case spex::IShipyard::SUCCESS:
      return Shipyard::eSuccess;
    case spex::IShipyard::INTERNAL_ERROR:
      return Shipyard::eInternalError;
    case spex::IShipyard::CARGO_NOT_FOUND:
      return Shipyard::eCargoNotFound;
    case spex::IShipyard::BUILD_STARTED:
      return Shipyard::eBuildStarted;
    case spex::IShipyard::BUILD_IN_PROGRESS:
      return Shipyard::eBuildInProgress;
    case spex::IShipyard::BUILD_COMPLETE:
      return Shipyard::eBuildComplete;
    case spex::IShipyard::BUILD_FROZEN:
      return Shipyard::eBuildFrozen;
    case spex::IShipyard::BUILD_FAILED:
      return Shipyard::eBuildFailed;
    case spex::IShipyard::BUILD_CANCELED:
      return Shipyard::eBuildCanceled;
    case spex::IShipyard::BLUEPRINT_NOT_FOUND:
      return Shipyard::eBlueprintNotFound;
    case spex::IShipyard::SHIPYARD_IS_BUSY:
      return Shipyard::eShipyardIsBusy;
    default: {
      // To avoid dunny warning
      assert("Unsupported status" == nullptr);
      return Shipyard::eStatusError;
    }
  }
}

bool Shipyard::getSpecification(ShipyardSpecification& spec)
{
  spex::Message request;
  request.mutable_shipyard()->set_specification_req(true);
  if (!send(request))
    return false;

  spex::IShipyard response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IShipyard::kSpecification)
    return false;

  spec.m_nLaborPerSec = response.specification().labor_per_sec();
  return true;
}

Shipyard::Status Shipyard::bindToCargo(std::string const& container)
{
  spex::Message request;
  spex::IShipyard* pBody = request.mutable_shipyard();
  pBody->set_bind_to_cargo(container);
  if (!send(request))
    return eTransportError;

  spex::IShipyard response;
  if (!wait(response))
    return eTimeoutError;
  if (response.choice_case() != spex::IShipyard::kBindToCargoStatus) {
    return eUnexpectedMessage;
  }
  return convert(response.bind_to_cargo_status());
}

Shipyard::Status Shipyard::startBuilding(std::string const& sBlueprint,
                                         std::string const& sShipName)
{
  spex::Message request;
  spex::IShipyard::StartBuild* pBody = request.mutable_shipyard()->mutable_start_build();
  pBody->set_blueprint_name(sBlueprint);
  pBody->set_ship_name(sShipName);
  if (!send(request))
    return eTransportError;

  spex::IShipyard response;
  if (!wait(response))
    return eTimeoutError;
  if (response.choice_case() != spex::IShipyard::kBuildingReport) {
    return eUnexpectedMessage;
  }
  return convert(response.building_report().status());
}

Shipyard::Status Shipyard::cancelBuild()
{
  spex::Message request;
  request.mutable_shipyard()->set_cancel_build(true);
  if (!send(request))
    return eTransportError;

  spex::IShipyard response;
  if (!wait(response))
    return eTimeoutError;
  if (response.choice_case() != spex::IShipyard::kBuildingReport) {
    return eUnexpectedMessage;
  }
  return convert(response.building_report().status());
}

Shipyard::Status Shipyard::waitingWhileBuilding(
    double *progress,
    uint32_t* pSlotId,
    std::string* pShipName)
{
  while (true) {
    spex::IShipyard response;
    if (!wait(response))
      return eTimeoutError;
    if (response.choice_case() != spex::IShipyard::kBuildingReport) {
      return eUnexpectedMessage;
    }

    Shipyard::Status status = convert(response.building_report().status());
    if (progress) {
      *progress = response.building_report().progress();
    }

    if (status == Shipyard::Status::eBuildInProgress) {
      continue;
    } else if (status != Shipyard::Status::eBuildComplete) {
      return status;
    }
    // Got report with 'BUILD_COMPLETE' status
    // Waiting for 'building_complete' message as well
    if (!wait(response))
      return eTimeoutError;
    if (response.choice_case() != spex::IShipyard::kBuildingComplete) {
      return eUnexpectedMessage;
    }
    if (pSlotId)
      *pSlotId = response.building_complete().slot_id();
    if (pShipName)
      *pShipName = response.building_complete().ship_name();
    return Shipyard::eSuccess;
  }
}

}}  // namespace autotests::client

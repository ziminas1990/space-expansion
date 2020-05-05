#include "ClientShipyard.h"

namespace autotests { namespace client {

Shipyard::Status convert(spex::IShipyard::Status eStatus)
{
  switch (eStatus) {
    case spex::IShipyard::SUCCESS:
      return Shipyard::eSuccess;
    case spex::IShipyard::INTERNAL_ERROR:
      return Shipyard::eInternalError;
    case spex::IShipyard::BUILD_STARTED:
      return Shipyard::eBuildStarted;
    case spex::IShipyard::BUILD_FREEZED:
      return Shipyard::eBuildFreezed;
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
  if (response.choice_case() != spex::IShipyard::kBuildingStatus) {
    return eUnexpectedMessage;
  }
  return convert(response.building_status());
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
  if (response.choice_case() != spex::IShipyard::kBuildingStatus) {
    return eUnexpectedMessage;
  }
  return convert(response.building_status());
}

Shipyard::Status Shipyard::waitingWhileBuilding(
    uint32_t* pSlotId, std::string* pShipName)
{
  while (true) {
    spex::IShipyard response;
    if (!wait(response))
      return eTimeoutError;
    if (response.choice_case() == spex::IShipyard::kBuildingProgress) {
      continue;
    }
    if (response.choice_case() == spex::IShipyard::kBuildingStatus) {
      return convert(response.building_status());
    }
    if (response.choice_case() == spex::IShipyard::kBuildingComplete) {
      if (pSlotId)
        *pSlotId = response.building_complete().slot_id();
      if (pShipName)
        *pShipName = response.building_complete().ship_name();
      return Shipyard::eSuccess;
    }
    return Shipyard::eUnexpectedMessage;
  }
}

}}  // namespace autotests::client

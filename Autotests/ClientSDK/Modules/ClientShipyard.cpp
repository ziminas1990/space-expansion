#include "ClientShipyard.h"

namespace autotests { namespace client {

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

}}  // namespace autotests::client

#include "ClientRCS.h"

namespace autotests { namespace client {

bool RCS::getSpecification(RCSSpecification& specification)
{
  spex::Message request;
  request.mutable_rcs()->set_specification_req(true);
  if (!send(std::move(request)))
    return false;

  spex::IRCS response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IRCS::kSpecification)
    return false;

  specification.nMaxThrust = response.specification().max_thrust();
  return true;
}

bool RCS::setThrust(geometry::Vector direction, uint32_t nDurationMs,
                       uint64_t nWhenUs)
{
  spex::Message request;
  if (nWhenUs) {
    request.set_timestamp(nWhenUs);
  }
  spex::IRCS::ChangeThrust *pBody = request.mutable_rcs()->mutable_change_thrust();
  pBody->set_x(direction.getX());
  pBody->set_y(direction.getY());
  pBody->set_duration_ms(nDurationMs);
  return send(std::move(request));
}

bool RCS::getThrust(geometry::Vector &thrust)
{
  spex::Message request;
  request.mutable_rcs()->set_thrust_req(true);
  if (!send(std::move(request)))
    return false;

  return waitThrust(thrust);
}

bool RCS::monitor(geometry::Vector &thrust)
{
  spex::Message request;
  request.mutable_rcs()->set_monitor(true);
  return send(std::move(request)) && waitThrust(thrust);
}

bool RCS::waitThrust(geometry::Vector &thrust, uint16_t nTimeout)
{
  spex::IRCS response;
  if (!wait(response, nTimeout))
    return false;
  if (response.choice_case() != spex::IRCS::kThrust)
    return false;

  spex::IRCS::CurrentThrust const& currentThrust = response.thrust();
  if (!currentThrust.thrust()) {
    thrust.toZero();
    return true;
  }
  thrust.setPosition(currentThrust.x(), currentThrust.y());
  thrust.setLength(currentThrust.thrust());
  return true;
}

}}  // namespace autotests::client

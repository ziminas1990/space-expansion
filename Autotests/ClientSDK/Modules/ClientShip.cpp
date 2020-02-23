#include "ClientShip.h"

#include <Protocol.pb.h>

namespace autotests { namespace client {

bool Ship::getPosition(geometry::Point& position, geometry::Vector& velocity)
{
  spex::Message request;
  request.mutable_navigation()->set_position_req(true);
  if (!send(request))
    return false;

  spex::INavigation response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::INavigation::kPosition)
    return false;

  position.x = response.position().x();
  position.y = response.position().y();
  velocity.setPosition(response.position().vx(),
                       response.position().vy());
  return true;
}

bool Ship::getPosition(geometry::Point &position)
{
  geometry::Vector velocity;
  return getPosition(position, velocity);
}

bool Ship::getState(ShipState& state)
{
  spex::Message request;
  request.mutable_ship()->set_state_req(true);
  if (!send(request))
    return false;

  spex::IShip response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IShip::kState)
    return false;

  state.nWeight = response.state().weight();
  return true;
}

}}  // namespace autotests::client

#include "ClientShip.h"

#include <Protocol.pb.h>

namespace autotests { namespace client {

bool Ship::getPosition(geometry::Point& position, geometry::Vector& velocity)
{
  spex::Message request;
  request.mutable_navigation()->set_position_req(true);
  if (!send(std::move(request)))
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

bool Ship::monitor(uint32_t nPeriodMs, ShipState &state)
{
  spex::Message request;
  request.mutable_ship()->set_monitor(nPeriodMs);
  return send(std::move(request)) && waitState(state);
}

bool Ship::waitState(ShipState &state, uint16_t nTimeout)
{
  spex::IShip response;
  if (!wait(response, nTimeout))
    return false;
  if (response.choice_case() != spex::IShip::kState)
    return false;

  if (response.state().has_weight()) {
    state.nWeight = response.state().weight().value();
  }
  if (response.state().has_position()) {
    const spex::Position& position = response.state().position();
    state.position.x = position.x();
    state.position.y = position.y();
    state.velocity.setPosition(position.vx(), position.vy());
  }
  return true;
}

bool Ship::getState(ShipState& state)
{
  spex::Message request;
  request.mutable_ship()->set_state_req(true);
  return send(std::move(request)) && waitState(state);
}

}}  // namespace autotests::client

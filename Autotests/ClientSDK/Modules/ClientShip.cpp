#include "ClientShip.h"

#include <Protocol.pb.h>

namespace autotests { namespace client {

bool Ship::getPosition(geometry::Point& position, geometry::Vector& velocity)
{
  spex::Message request;
  request.mutable_navigation()->mutable_positionrequest();
  if (!send(request))
    return false;

  spex::INavigation response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::INavigation::kPositionResponse)
    return false;

  position.x = response.positionresponse().x();
  position.y = response.positionresponse().y();
  velocity.setPosition(response.positionresponse().vx(),
                       response.positionresponse().vy());
  return true;
}

bool Ship::getPosition(geometry::Point &position)
{
  geometry::Vector velocity;
  return getPosition(position, velocity);
}

}}  // namespace autotests::client

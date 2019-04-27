#include "CommandCenter.h"

namespace modules {

void CommandCenter::handleNavigationMessage(
    size_t nSessionId, spex::INavigation const& message)
{
  switch (message.choice_case()) {
    case spex::INavigation::kPositionRequest: {
      spex::INavigation navigation;
      spex::INavigation_GetPositionResponse* pBody =
          navigation.mutable_positionresponse();
      pBody->set_x(getPosition().x);
      pBody->set_y(getPosition().y);
      pBody->set_vx(getVelocity().getPosition().x);
      pBody->set_vy(getVelocity().getPosition().y);
      send(nSessionId, std::move(navigation));
      break;
    }
    default: {
      break;
    }
  }
}


} // namespace modules

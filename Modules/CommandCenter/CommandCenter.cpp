#include "CommandCenter.h"

namespace modules {

void CommandCenter::handleMessage(spex::CommandCenterMessage const& message)
{
  switch (message.choice_case()) {
    case spex::CommandCenterMessage::kNavigation: {
      onNavigationMessage(message.navigation());
      break;
    }
    default: {
      break;
    }
  }
}

void CommandCenter::onNavigationMessage(spex::INavigation const& message)
{
  switch (message.choice_case()) {
    case spex::INavigation::kPositionRequest: {
      network::IProtobufChannelPtr pChannel = m_pChannel.lock();
      if (!pChannel) {
        m_pChannel.reset();
        return;
      }

      spex::CommandCenterMessage response;
      spex::INavigation_GetPositionResponse* pBody =
          response.mutable_navigation()->mutable_positionresponse();
      pBody->set_x(getPosition().x);
      pBody->set_y(getPosition().y);
      pBody->set_vx(getVelocity().getPosition().x);
      pBody->set_vy(getVelocity().getPosition().y);
      pChannel->sendMessage(response);
      break;
    }
    default: {
      break;
    }
  }
}


} // namespace modules

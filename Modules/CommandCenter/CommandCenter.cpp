#include "CommandCenter.h"

namespace modules {

void CommandCenter::handleMessage(size_t nSessionId,
                                  spex::CommandCenterMessage &&message)
{
  switch (message.choice_case()) {
    case spex::CommandCenterMessage::kNavigation: {
      onNavigationMessage(nSessionId, message.navigation());
      break;
    }
    default: {
      break;
    }
  }
}

void CommandCenter::onNavigationMessage(
    size_t nSessionId, spex::INavigation const& message)
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
      pChannel->sendMessage(nSessionId, response);
      break;
    }
    default: {
      break;
    }
  }
}


} // namespace modules

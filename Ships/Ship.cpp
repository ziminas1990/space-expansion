#include "Ship.h"

namespace ships
{

Ship::Ship(std::string const& sShipType)
  : BaseModule(std::string("Ship/") + sShipType)
{
  m_Modules.reserve(0x0F);
}

Ship::~Ship()
{
  for (modules::BaseModulePtr& pModule : m_Modules)
    pModule->onDoestroyed();
}

void Ship::installModule(modules::BaseModulePtr pModule)
{
  m_Modules.push_back(pModule);
  m_pCommutator->attachModule(pModule);
}

void Ship::onMessageReceived(uint32_t nSessionId, spex::Message const& message)
{
  if (message.choice_case() == spex::Message::kCommutator) {
    // Forwarding message to commutator
    m_pCommutator->onMessageReceived(nSessionId, message);
  } else {
    BaseModule::onMessageReceived(nSessionId, message);
  }
}

void Ship::handleNavigationMessage(uint32_t nSessionId, spex::INavigation const& message)
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
      sendToClient(nSessionId, navigation);
      break;
    }
    default: {
      break;
    }
  }
}

} // namespace modules
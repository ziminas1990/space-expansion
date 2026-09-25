#include "HoverEngine.h"
#include <Modules/Ship/Ship.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::HoverEngine);

namespace modules {

HoverEngine::HoverEngine(std::string&& sName, world::PlayerWeakPtr pOwner,
                         uint32_t maxThrust)
  : BaseModule("HoverEngine", std::move(sName), std::move(pOwner)),
    m_maxThrust(maxThrust)
{
  GlobalObject<HoverEngine>::registerSelf(this);
}

void HoverEngine::proceed(uint32_t nIntervalUs)
{
  if (m_nTimeLeftUs > nIntervalUs) {
    m_nTimeLeftUs -= nIntervalUs;
    return;
  }

  m_nTimeLeftUs = 0;
  if (!m_thrust) {
    switchToIdleState();
    return;
  }

  m_thrust = 0;
  applyThrust();
  switchToIdleState();
  notifyMonitors();
}

void HoverEngine::onSessionClosed(uint32_t nSessionId)
{
  m_monitoringSessions.removeFirst(nSessionId);
  BaseModule::onSessionClosed(nSessionId);
}

void HoverEngine::handleHoverEngineMessage(
    uint32_t nSessionId, spex::IHoverEngine const& message)
{
  switch (message.choice_case()) {
    case spex::IHoverEngine::kSpecificationReq: {
      getSpecification(nSessionId);
      return;
    }
    case spex::IHoverEngine::kChangeThrust: {
      setThrust(message.change_thrust());
      return;
    }
    case spex::IHoverEngine::kThrustReq: {
      sendThrust(nSessionId);
      return;
    }
    case spex::IHoverEngine::kMonitor: {
      monitor(nSessionId);
      return;
    }
    case spex::IHoverEngine::kSpecification:
    case spex::IHoverEngine::kThrust:
    case spex::IHoverEngine::CHOICE_NOT_SET:
      assert(!"Unexpected message");
      return;
  }
}

void HoverEngine::onInstalled(modules::Ship* pPlatform)
{
  m_nForceId = pPlatform->allocateForce(true);
}

void HoverEngine::getSpecification(uint32_t nSessionId) const
{
  spex::Message response;
  spex::IHoverEngine* pBody = response.mutable_hover_engine();
  pBody->mutable_specification()->set_max_thrust(m_maxThrust);
  sendToClient(nSessionId, std::move(response));
}

void HoverEngine::setThrust(spex::IHoverEngine::ChangeThrust const& req)
{
  const uint32_t thrust = std::min(req.thrust(), m_maxThrust);
  if (thrust == 0) {
    m_thrust = 0;
    m_nTimeLeftUs = 0;
    applyThrust();
    switchToIdleState();
  } else {
    m_thrust = thrust;
    m_nTimeLeftUs = req.duration_ms() * 1000;
    applyThrust();
    switchToActiveState();
  }
  notifyMonitors();
}

void HoverEngine::monitor(uint32_t nSessionId)
{
  m_monitoringSessions.push(nSessionId);
  sendThrust(nSessionId);
}

bool HoverEngine::sendThrust(uint32_t nSessionId) const
{
  spex::Message response;
  response.mutable_hover_engine()->set_thrust(m_thrust);
  return sendToClient(nSessionId, std::move(response));
}

void HoverEngine::notifyMonitors()
{
  for (size_t i = 0; i < m_monitoringSessions.size();) {
    if (!sendThrust(m_monitoringSessions[i])) {
      m_monitoringSessions.remove(i);
    } else {
      ++i;
    }
  }
}

void HoverEngine::applyThrust()
{
  geometry::Vector const& nose = getPlatform()->getOrientation();
  getPlatform()->getForce(m_nForceId).setPosition(
      nose.getX() * m_thrust, nose.getY() * m_thrust);
}

} // namespace modules

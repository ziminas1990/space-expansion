#include "RCS.h"
#include <Modules/Ship/Ship.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::RCS);

namespace modules {

RCS::RCS(std::string&& sName, world::PlayerWeakPtr pOwner, uint32_t maxThrust)
  : BaseModule ("RCS", std::move(sName), std::move(pOwner)),
    m_maxThrust(maxThrust)
{
  GlobalObject<RCS>::registerSelf(this);
}

void RCS::proceed(uint32_t nIntervalUs)
{
  if (m_nTimeLeftUs > nIntervalUs) {
    m_nTimeLeftUs -= nIntervalUs;
    return;
  }

  geometry::Vector& thrustVector =
      getPlatform()->getExternalForce_NoSync(m_nThrustVectorId);
  thrustVector.toZero();
  switchToIdleState();
  m_nTimeLeftUs = 0;
  notifyMonitors();
}

bool RCS::loadState(YAML::Node const& source)
{
  if (!BaseModule::loadState(source))
    return false;

  geometry::Vector& thrust = getPlatform()->getExternalForce_NoSync(m_nThrustVectorId);
  return thrust.load(source);
}

void RCS::onSessionClosed(uint32_t nSessionId)
{
  m_monitoringSessions.removeFirst(nSessionId);
  BaseModule::onSessionClosed(nSessionId);
}

void RCS::handleRCSMessage(uint32_t nSessionId, spex::IRCS const& message)
{
  switch(message.choice_case()) {
    case spex::IRCS::kSpecificationReq: {
      getSpecification(nSessionId);
      return;
    }
    case spex::IRCS::kChangeThrust: {
      setThrust(message.change_thrust());
      return;
    }
    case spex::IRCS::kThrustReq: {
      getThrust(nSessionId);
      return;
    }
    case spex::IRCS::kMonitor: {
      monitor(nSessionId);
      return;
    }
    case spex::IRCS::kThrust:
    case spex::IRCS::kSpecification:
    case spex::IRCS::CHOICE_NOT_SET:
      assert("Unexpected message" == nullptr);
      return;
  }
}

void RCS::onInstalled(modules::Ship* pPlatform)
{
  m_nThrustVectorId = pPlatform->createExternalForce();
}

void RCS::getSpecification(uint32_t nSessionId) const
{
  spex::Message response;
  spex::IRCS* pBody = response.mutable_rcs();
  pBody->mutable_specification()->set_max_thrust(m_maxThrust);
  sendToClient(nSessionId, std::move(response));
}

void RCS::setThrust(const spex::IRCS::ChangeThrust &req)
{
  geometry::Vector& thrustVector =
      getPlatform()->getExternalForce_NoSync(m_nThrustVectorId);

  uint32_t thrust = req.thrust();
  if (!thrust) {
    thrustVector.toZero();
    m_nTimeLeftUs = 0;
    switchToIdleState();
  } else {
    thrustVector.setPosition(req.x(), req.y());
    if (thrust > m_maxThrust)
      thrust = m_maxThrust;
    thrustVector.setLength(thrust);
    m_nTimeLeftUs = req.duration_ms() * 1000;
    switchToActiveState();
  }

  notifyMonitors();
}

void RCS::getThrust(uint32_t nSessionId) const
{
  sendThrust(nSessionId);
}

void RCS::monitor(uint32_t nSessionId)
{
  m_monitoringSessions.push(nSessionId);
  sendThrust(nSessionId);
}

bool RCS::sendThrust(uint32_t nSessionId) const
{
  geometry::Vector const& thrustVector =
      getPlatform()->getExternalForce_NoSync(m_nThrustVectorId);

  spex::Message response;
  spex::IRCS::CurrentThrust* pBody =
      response.mutable_rcs()->mutable_thrust();
  pBody->set_x(thrustVector.getX());
  pBody->set_y(thrustVector.getY());
  pBody->set_thrust(uint32_t(thrustVector.getLength()));
  return sendToClient(nSessionId, std::move(response));
}

void RCS::notifyMonitors()
{
  for (size_t i = 0; i < m_monitoringSessions.size();) {
    if (!sendThrust(m_monitoringSessions[i])) {
      m_monitoringSessions.remove(i);
    } else {
      ++i;
    }
  }
}

} // namespace modules

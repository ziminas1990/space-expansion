#include "Engine.h"
#include <Ships/Ship.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Engine);

namespace modules {

Engine::Engine(std::string&& sName, world::PlayerWeakPtr pOwner, uint32_t maxThrust)
  : BaseModule ("Engine", std::move(sName), std::move(pOwner)),
    m_maxThrust(maxThrust)
{
  GlobalContainer<Engine>::registerSelf(this);
}

void Engine::proceed(uint32_t nIntervalUs)
{
  if (m_nTimeLeftUs > nIntervalUs) {
    m_nTimeLeftUs -= nIntervalUs;
    return;
  }
  getPlatform()->getExternalForce_NoSync(m_nThrustVectorId).toZero();
  switchToIdleState();
  m_nTimeLeftUs = 0;
}

bool Engine::loadState(YAML::Node const& source)
{
  if (!BaseModule::loadState(source))
    return false;

  geometry::Vector& thrust = getPlatform()->getExternalForce_NoSync(m_nThrustVectorId);
  return thrust.load(source);
}

void Engine::handleEngineMessage(uint32_t nSessionId, spex::IEngine const& message)
{
  switch(message.choice_case()) {
    case spex::IEngine::kGetSpecification: {
      getSpecification(nSessionId);
      return;
    }
    case spex::IEngine::kSetThrust: {
      setThrust(message.setthrust());
      return;
    }
    case spex::IEngine::kGetThrust: {
      getThrust(nSessionId);
      return;
    }
    case spex::IEngine::kCurrentThrust:
    case spex::IEngine::kSpecification:
    case spex::IEngine::CHOICE_NOT_SET:
      return;
  }
}

void Engine::onInstalled(ships::Ship* pPlatform)
{
  m_nThrustVectorId = pPlatform->createExternalForce();
}

void Engine::getSpecification(uint32_t nSessionId) const
{
  spex::IEngine response;
  response.mutable_specification()->set_maxthrust(m_maxThrust);
  sendToClient(nSessionId, response);
}

void Engine::setThrust(spex::IEngine::SetThrust const& req)
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
}

void Engine::getThrust(uint32_t nSessionId) const
{
  spex::IEngine response;
  spex::IEngine::CurrentThrust *pBody = response.mutable_currentthrust();

  geometry::Vector const& thrustVector =
      getPlatform()->getExternalForce_NoSync(m_nThrustVectorId);
  pBody->set_x(thrustVector.getX());
  pBody->set_y(thrustVector.getY());
  pBody->set_thrust(uint32_t(thrustVector.getLength()));

  sendToClient(nSessionId, response);
}

} // namespace modules

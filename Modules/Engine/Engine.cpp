#include "Engine.h"

DECLARE_GLOBAL_CONTAINER_CPP(modules::Engine);

namespace modules {

Engine::Engine(uint32_t maxThrust)
  : BaseModule ("Engine/Nuclear"),
    m_maxThrust(maxThrust)
{
  GlobalContainer<Engine>::registerSelf(this);
}

void Engine::installOn(newton::PhysicalObject *pPlatform)
{
  m_pPlatform       = pPlatform;
  m_nThrustVectorId = m_pPlatform->createExternalForce();
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

void Engine::getSpecification(uint32_t nSessionId) const
{
  spex::IEngine response;
  response.mutable_specification()->set_maxthrust(m_maxThrust);
  sendToClient(nSessionId, response);
}

void Engine::setThrust(spex::IEngine::SetThrust const& req)
{
  geometry::Vector& thrustVector =
      m_pPlatform->getExternalForce_NoSync(m_nThrustVectorId);

  uint32_t thrust = req.thrust();
  if (!thrust) {
    thrustVector.toZero();
  } else {
    thrustVector.setPosition(req.x(), req.y());
    if (thrust > m_maxThrust)
      thrust = m_maxThrust;
    thrustVector.setLength(thrust);
  }
}

void Engine::getThrust(uint32_t nSessionId) const
{
  spex::IEngine response;
  spex::IEngine::CurrentThrust *pBody = response.mutable_currentthrust();

  geometry::Vector const& thrustVector =
      m_pPlatform->getExternalForce_NoSync(m_nThrustVectorId);
  pBody->set_x(thrustVector.getPosition().x);
  pBody->set_y(thrustVector.getPosition().y);
  pBody->set_y(thrustVector.getLength());

  sendToClient(nSessionId, response);
}

} // namespace modules

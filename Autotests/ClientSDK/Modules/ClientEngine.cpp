#include "ClientEngine.h"

namespace autotests { namespace client {

bool Engine::getSpecification(EngineSpecification& specification)
{
  spex::Message request;
  request.mutable_engine()->mutable_getspecification();
  if (!send(request))
    return false;

  spex::IEngine response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IEngine::kSpecification)
    return false;

  specification.nMaxThrust = response.specification().maxthrust();
  return true;
}

bool Engine::setThrust(geometry::Vector thrust, uint32_t nDurationMs)
{
  spex::Message request;
  spex::IEngine::SetThrust *pBody = request.mutable_engine()->mutable_setthrust();
  pBody->set_x(thrust.getX());
  pBody->set_y(thrust.getY());
  pBody->set_thrust(uint32_t(thrust.getLength()));
  pBody->set_duration_ms(nDurationMs);
  return send(request);
}

bool Engine::getThrust(geometry::Vector &thrust)
{
  spex::Message request;
  request.mutable_engine()->mutable_getthrust();
  if (!send(request))
    return false;

  spex::IEngine response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IEngine::kCurrentThrust)
    return false;

  spex::IEngine::CurrentThrust const& currentThrust = response.currentthrust();
  thrust.setPosition(currentThrust.x(), currentThrust.y());
  thrust.setLength(currentThrust.thrust());
  return true;
}

}}  // namespace autotests::client

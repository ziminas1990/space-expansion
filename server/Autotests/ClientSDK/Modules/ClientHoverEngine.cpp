#include "ClientHoverEngine.h"

namespace autotests { namespace client {

bool HoverEngine::getSpecification(HoverEngineSpecification& specification)
{
  spex::Message request;
  request.mutable_hover_engine()->set_specification_req(true);
  if (!send(std::move(request)))
    return false;

  spex::IHoverEngine response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::IHoverEngine::kSpecification)
    return false;

  specification.nMaxThrust = response.specification().max_thrust();
  return true;
}

bool HoverEngine::setThrust(uint32_t nThrust, uint32_t nDurationMs,
                            uint64_t nWhenUs)
{
  spex::Message request;
  if (nWhenUs) {
    request.set_timestamp(nWhenUs);
  }
  spex::IHoverEngine::ChangeThrust* pBody =
      request.mutable_hover_engine()->mutable_change_thrust();
  pBody->set_thrust(nThrust);
  pBody->set_duration_ms(nDurationMs);
  return send(std::move(request));
}

bool HoverEngine::getThrust(uint32_t& nThrust)
{
  spex::Message request;
  request.mutable_hover_engine()->set_thrust_req(true);
  if (!send(std::move(request)))
    return false;

  return waitThrust(nThrust);
}

bool HoverEngine::monitor(uint32_t& nThrust)
{
  spex::Message request;
  request.mutable_hover_engine()->set_monitor(true);
  return send(std::move(request)) && waitThrust(nThrust);
}

bool HoverEngine::waitThrust(uint32_t& nThrust, uint16_t nTimeout)
{
  spex::IHoverEngine response;
  if (!wait(response, nTimeout))
    return false;
  if (response.choice_case() != spex::IHoverEngine::kThrust)
    return false;

  nThrust = response.thrust();
  return true;
}

}}  // namespace autotests::client

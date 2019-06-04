#include "MockedCommutator.h"

namespace autotests
{

MockedCommutator::MockedCommutator()
  : MockedBaseModule("MockedCommutator")
{}

bool MockedCommutator::waitOpenTunnel(uint32_t nSessionId, uint32_t nSlotId)
{
  spex::ICommutator message;
  if (!wait(nSessionId, message))
    return false;
  if (message.choice_case() != spex::ICommutator::kOpenTunnel)
    return false;
  return message.opentunnel().nslotid() == nSlotId;
}

bool MockedCommutator::sendOpenTunnelSuccess(uint32_t nSessionId, uint32_t nTunnelId)
{
  spex::ICommutator message;
  spex::ICommutator::OpenTunnelSuccess* pBody = message.mutable_opentunnelsuccess();
  pBody->set_ntunnelid(nTunnelId);
  return sendToClient(nSessionId, message);
}

bool MockedCommutator::sendOpenTunnelFailed(uint32_t nSessionId)
{
  spex::ICommutator message;
  message.mutable_opentunnelfailed();
  return sendToClient(nSessionId, message);
}

} // namespace autotests

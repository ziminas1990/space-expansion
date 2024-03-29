#include "MockedCommutator.h"
#include <Protocol.pb.h>

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
  return message.open_tunnel() == nSlotId;
}

bool MockedCommutator::sendOpenTunnelSuccess(uint32_t nSessionId, uint32_t nTunnelId)
{
  spex::Message message;
  message.mutable_commutator()->set_open_tunnel_report(nTunnelId);
  return sendToClient(nSessionId, std::move(message));
}

bool MockedCommutator::sendOpenTunnelFailed(uint32_t nSessionId,
                                            spex::ICommutator::Status eReason)
{
  spex::Message message;
  message.mutable_commutator()->set_open_tunnel_failed(eReason);
  return sendToClient(nSessionId, std::move(message));
}

bool MockedCommutator::waitTotalSlotsReq(uint32_t nSessionId)
{
  spex::ICommutator message;
  if (!wait(nSessionId, message))
    return false;
  return message.choice_case() == spex::ICommutator::kTotalSlotsReq;
}

bool MockedCommutator::sendTotalSlots(uint32_t nSessionId, uint32_t slots)
{
  spex::Message message;
  message.mutable_commutator()->set_total_slots(slots);
  return sendToClient(nSessionId, std::move(message));
}

} // namespace autotests

#include "MockedCommutator.h"

namespace autotests
{

//========================================================================================
// CommutatorClient
//========================================================================================

bool CommutatorClient::sendGetTotalSlots(uint32_t nSessionId, uint32_t nExpectedSlots)
{
  spex::Message request;
  request.mutable_commutator()->mutable_gettotalslots();
  m_pSyncPipe->onMessageReceived(nSessionId, std::move(request));

  spex::ICommutator message;
  if (!m_pSyncPipe->wait(nSessionId, message))
    return false;
  if (message.choice_case() != spex::ICommutator::kTotalSlotsResponse)
    return false;
  return message.totalslotsresponse().ntotalslots() == nExpectedSlots;
}

bool CommutatorClient::openTunnel(
    uint32_t nSessionId, uint32_t nSlotId, bool lExpectSuccess, uint32_t* pOpenedTunnelId)
{
  spex::Message request;
  request.mutable_commutator()->mutable_opentunnel()->set_nslotid(nSlotId);
  m_pSyncPipe->onMessageReceived(nSessionId, std::move(request));

  spex::ICommutator message;
  if (!m_pSyncPipe->wait(nSessionId, message))
    return false;
  if (lExpectSuccess) {
    if (message.choice_case() != spex::ICommutator::kOpenTunnelSuccess)
      return false;
    if (pOpenedTunnelId) {
      *pOpenedTunnelId = message.opentunnelsuccess().ntunnelid();
    }
    return true;
  } else {
    return message.choice_case() != spex::ICommutator::kOpenTunnelFailed;
  }
}

//========================================================================================
// MockedCommutator
//========================================================================================

MockedCommutator::MockedCommutator()
  : ProtobufSyncPipe(ProtobufSyncPipe::eMockedTerminalMode),
    modules::BaseModule("MockedCommutator")
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

} // namespace autotests

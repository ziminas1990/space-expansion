#include "MockedCommutator.h"

namespace autotests
{

//========================================================================================
// CommutatorClient
//========================================================================================

bool ClientCommutator::getTotalSlots(uint32_t nExpectedSlots)
{
  spex::Message request;
  request.mutable_commutator()->mutable_gettotalslots();
  if (!m_pSyncPipe->send(m_nTunnelId, std::move(request)))
    return false;

  spex::ICommutator message;
  if (!m_pSyncPipe->wait(m_nTunnelId, message))
    return false;
  if (message.choice_case() != spex::ICommutator::kTotalSlotsResponse)
    return false;
  return message.totalslotsresponse().ntotalslots() == nExpectedSlots;
}

bool ClientCommutator::getAttachedModulesList(
    uint32_t nTotal, ModulesList& attachedModules)
{
  spex::Message request;
  request.mutable_commutator()->mutable_getallmodulesinfo();
  m_pSyncPipe->send(m_nTunnelId, std::move(request));

  for (size_t i = 0; i < nTotal; ++i) {
    spex::ICommutator response;
    if (!m_pSyncPipe->wait(m_nTunnelId, response))
      return false;
    if (response.choice_case() != spex::ICommutator::kModuleInfo)
      return false;
    attachedModules.push_back(
          ModuleInfo({response.moduleinfo().nslotid(),
                      response.moduleinfo().smoduletype()}));
  }

  return attachedModules.size() == nTotal;
}

bool ClientCommutator::openTunnel(uint32_t nSlotId, bool lExpectSuccess,
                                  uint32_t* pOpenedTunnelId)
{
  return sendOpenTunnel(nSlotId)
      && lExpectSuccess ? waitOpenTunnelSuccess(pOpenedTunnelId)
                        : waitOpenTunnelFailed();
}

bool ClientCommutator::sendOpenTunnel(uint32_t nSlotId)
{
  spex::Message request;
  request.mutable_commutator()->mutable_opentunnel()->set_nslotid(nSlotId);
  return m_pSyncPipe->send(m_nTunnelId, request);
}

bool ClientCommutator::waitOpenTunnelSuccess(uint32_t *pOpenedTunnelId)
{
  spex::ICommutator message;
  if (!m_pSyncPipe->wait(m_nTunnelId, message))
    return false;
  if (message.choice_case() != spex::ICommutator::kOpenTunnelSuccess)
    return false;
  if (pOpenedTunnelId) {
    *pOpenedTunnelId = message.opentunnelsuccess().ntunnelid();
  }
  return true;
}

bool ClientCommutator::waitOpenTunnelFailed()
{
  spex::ICommutator message;
  return m_pSyncPipe->wait(m_nTunnelId, message)
      && message.choice_case() == spex::ICommutator::kOpenTunnelFailed;
}

//========================================================================================
// MockedCommutator
//========================================================================================

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

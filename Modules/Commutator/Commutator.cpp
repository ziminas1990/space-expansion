#include "Commutator.h"
#include<Protocol.pb.h>

namespace modules
{

void Commutator::attachModule(BaseModulePtr pModule)
{
  // We assume, that this operation is rather rare therefore we can afford to
  // execute it in O(N) time
  for (uint32_t nSlotId = 0; nSlotId < m_Slots.size(); ++nSlotId)
  {
    if (m_Slots[nSlotId]->isDestroyed())
    {
      onModuleHasBeenDetached(nSlotId);
      m_Slots[nSlotId] = pModule;
      return;
    }
  }
  m_Slots.push_back(pModule);
}

void Commutator::detachFromModules()
{
  for (Tunnel& tunnel : m_Tunnels) {
    m_Slots[tunnel.m_nSlotId]->onSessionClosed(tunnel.m_nSessionId);
    tunnel.m_lUp = false;
  }
  m_Tunnels.clear();
  for (BaseModulePtr& pModule : m_Slots) {
    pModule->detachFromChannel();
  }
}

void Commutator::checkSlotsAndTunnels()
{
  for (uint32_t nTunnelId = 0; nTunnelId < m_Tunnels.size(); ++nTunnelId)
  {
    Tunnel& tunnel = m_Tunnels[nTunnelId];
    if (!tunnel.m_lUp)
      continue;
    BaseModulePtr const& pModule = m_Slots[tunnel.m_nSlotId];
    if (!pModule || !pModule->isOnline()) {
      spex::ICommutator message;
      message.mutable_closetunnel()->set_ntunnelid(nTunnelId);
      sendToClient(tunnel.m_nSessionId, std::move(message));
      tunnel = Tunnel();
    }
  }

  for (uint32_t nSlotId = 0; nSlotId < m_Slots.size(); ++nSlotId)
  {
    if (m_Slots[nSlotId]->isDestroyed())
    {
      onModuleHasBeenDetached(nSlotId);
      m_Slots[nSlotId].reset();
    }
  }
}

bool Commutator::openSession(uint32_t nSessionId)
{
  if (m_OpenedSessions.size() >= m_nSessionsLimit)
    return false;
  m_OpenedSessions.insert(nSessionId);
  return true;
}

void Commutator::onSessionClosed(uint32_t nSessionId)
{
  m_OpenedSessions.erase(nSessionId);
  for (uint32_t nTunnelId = 0; nTunnelId <= m_Tunnels.size(); ++nTunnelId)
  {
    if (m_Tunnels[nTunnelId].m_nSessionId == nSessionId) {
      m_ReusableTunnels.push(nTunnelId);
      m_Tunnels[nTunnelId] = Tunnel();
    }
  }
}

bool Commutator::send(uint32_t nTunnelId, spex::Message const& message) const
{
  // in this context, sessionId (we got it from terminal) is a tunnelId
  spex::Message tunnelPDU;
  tunnelPDU.set_tunnelid(nTunnelId);
  *tunnelPDU.mutable_encapsulated() = message;
  return nTunnelId < m_Tunnels.size()
      && sendToClient(m_Tunnels[nTunnelId].m_nSessionId, tunnelPDU);
}

void Commutator::closeSession(uint32_t nSessionId)
{
  // in this context, sessionId (we got it from terminal) is a tunnelId
  onCloseTunnelRequest(nSessionId);
}

void Commutator::detachFromTerminal()
{
  detachFromModules();
}

void Commutator::handleMessage(uint32_t nSessionId, spex::Message const& message)
{
  if (message.choice_case() == spex::Message::kEncapsulated) {
    commutateMessage(message.tunnelid(), message.encapsulated());

  } else if (message.choice_case() == spex::Message::kCommutator) {
    spex::ICommutator const& commutatorPdu = message.commutator();
    switch(commutatorPdu.choice_case()) {
      case spex::ICommutator::kGetTotalSlots:
        onGetTotalSlotsRequest(nSessionId);
        return;
      case spex::ICommutator::kGetModuleInfo:
        getModuleInfo(nSessionId, commutatorPdu.getmoduleinfo().nslotid());
        return;
      case spex::ICommutator::kGetAllModulesInfo:
        getAllModulesInfo(nSessionId);
        return;
      case spex::ICommutator::kOpenTunnel:
        onOpenTunnelRequest(nSessionId, commutatorPdu.opentunnel().nslotid());
        return;
      case spex::ICommutator::kCloseTunnel:
        onCloseTunnelRequest(nSessionId, commutatorPdu.closetunnel().ntunnelid());
        return;
      default:
        return;
    }
  }
}

void Commutator::onGetTotalSlotsRequest(uint32_t nSessionId) const
{
  spex::ICommutator message;
  message.mutable_totalslotsresponse()->set_ntotalslots(
        static_cast<uint32_t>(m_Slots.size()));
  sendToClient(nSessionId, std::move(message));
}

void Commutator::getModuleInfo(uint32_t nSessionId, uint32_t nSlotId) const
{
  spex::ICommutator message;
  message.mutable_moduleinfo()->set_nslotid(nSlotId);
  if (nSlotId >= m_Slots.size() || !m_Slots[nSlotId]) {
    message.mutable_moduleinfo()->set_smoduletype("empty");
  } else {
    message.mutable_moduleinfo()->set_smoduletype(m_Slots[nSlotId]->getModuleType());
  }
  sendToClient(nSessionId, std::move(message));
}

void Commutator::getAllModulesInfo(uint32_t nSessionId) const
{
  for (uint32_t nSlotId = 0; nSlotId < m_Slots.size(); ++nSlotId)
  {
    if (!m_Slots[nSlotId])
      continue;
    spex::ICommutator message;
    message.mutable_moduleinfo()->set_nslotid(nSlotId);
    message.mutable_moduleinfo()->set_smoduletype(m_Slots[nSlotId]->getModuleType());
    sendToClient(nSessionId, std::move(message));
  }
}

void Commutator::onOpenTunnelRequest(uint32_t nSessionId, uint32_t nSlot)
{
  spex::ICommutator message;
  if (nSlot >= m_Slots.size() || !m_Slots[nSlot]) {
    message.mutable_opentunnelfailed();
  } else {
    uint32_t nTunnelId;
    if (!m_ReusableTunnels.empty()) {
      nTunnelId = m_ReusableTunnels.top();
      m_ReusableTunnels.pop();
    } else {
      m_Tunnels.push_back(Tunnel());
      nTunnelId = static_cast<uint32_t>(m_Tunnels.size() - 1);
    }
    Tunnel& tunnel      = m_Tunnels[nTunnelId];
    tunnel.m_nSlotId    = nSlot;
    tunnel.m_nSessionId = nSessionId;
    tunnel.m_lUp        = true;
    message.mutable_opentunnelsuccess()->set_ntunnelid(nTunnelId);
  }
  sendToClient(nSessionId, message);
}

void Commutator::onCloseTunnelRequest(uint32_t nTunnelId, uint32_t nSessionId)
{
  if (nTunnelId >= m_Tunnels.size())
    return;

  Tunnel& tunnel = m_Tunnels[nTunnelId];
  if (!tunnel.m_lUp ||
      (tunnel.m_nSessionId != nSessionId && nSessionId != uint32_t(-1)))
    return;

  spex::ICommutator message;
  message.mutable_closetunnel()->set_ntunnelid(nTunnelId);
  sendToClient(tunnel.m_nSessionId, std::move(message));
  tunnel = Tunnel();
}

void Commutator::commutateMessage(uint32_t nTunnelId, spex::Message const& message)
{
  if (nTunnelId >= m_Tunnels.size())
    return;
  Tunnel& tunnel = m_Tunnels[nTunnelId];
  if (!tunnel.m_lUp || tunnel.m_nSlotId >= m_Slots.size())
    return;
  BaseModulePtr& pModule = m_Slots[tunnel.m_nSlotId];
  if (!pModule || !pModule->isOnline())
    return;
  pModule->onMessageReceived(nTunnelId, message);
}

void Commutator::onModuleHasBeenDetached(uint32_t nSlotId)
{
  m_Slots[nSlotId] = nullptr;
  // We assume, that this operation is rather rare therefore we can afford to
  // execute it in O(N) time
  for (uint32_t nTunnelId = 0; nTunnelId < m_Tunnels.size(); ++nTunnelId) {
    if (m_Tunnels[nTunnelId].m_nSlotId == nSlotId) {
      spex::ICommutator message;
      message.mutable_tunnelclosed()->set_ntunnelid(nTunnelId);
      sendToClient(m_Tunnels[nTunnelId].m_nSessionId, std::move(message));
    }
  }
}

void Commutator::onModuleHasBeenAttached(uint32_t nSlotId)
{
  for (uint32_t nSessionId : m_OpenedSessions) {
    spex::ICommutator message;
    message.mutable_moduleinfo()->set_nslotid(nSlotId);
    message.mutable_moduleinfo()->set_smoduletype(m_Slots[nSlotId]->getModuleType());
    sendToClient(nSessionId, std::move(message));
  }
}

} // namespace modules

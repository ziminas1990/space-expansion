#include "Commutator.h"
#include<Protocol.pb.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Commutator);

namespace modules
{

Commutator::Commutator() : BaseModule("Commutator", std::string(), world::PlayerWeakPtr())
{
  GlobalContainer<Commutator>::registerSelf(this);
}

uint32_t Commutator::attachModule(BaseModulePtr pModule)
{
  // We assume, that this operation is rather rare therefore we can afford to
  // execute it in O(N) time
  for (uint32_t nSlotId = 0; nSlotId < m_Slots.size(); ++nSlotId)
  {
    if (m_Slots[nSlotId]->isDestroyed())
    {
      onModuleHasBeenDetached(nSlotId);
      m_Slots[nSlotId] = pModule;
      return nSlotId;
    }
  }
  m_Slots.push_back(pModule);
  return static_cast<uint32_t>(m_Slots.size()) - 1;
}

BaseModulePtr Commutator::findModuleByName(std::string const& sName) const
{
  for (BaseModulePtr pModule : m_Slots) {
    if (pModule->getModuleName() == sName)
      return pModule;
  }
  return BaseModulePtr();
}

BaseModulePtr Commutator::findModuleByType(std::string const& sType) const
{
  for (BaseModulePtr pModule : m_Slots) {
    if (pModule->getModuleType() == sType)
      return pModule;
  }
  return BaseModulePtr();
}

void Commutator::detachFromModules()
{
  for (Tunnel& tunnel : m_Tunnels) {
    m_Slots[tunnel.m_nSlotId]->onSessionClosed(tunnel.m_nFather);
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
      message.set_close_tunnel_report(nTunnelId);
      sendToClient(tunnel.m_nFather, std::move(message));
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

void Commutator::broadcast(spex::Message const& message)
{
  for (uint32_t nSessionId : m_OpenedSessions) {
    sendToClient(nSessionId, message);
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
    Tunnel& tunnel = m_Tunnels[nTunnelId];
    if (tunnel.m_nFather == nSessionId) {
      m_ReusableTunnels.push(nTunnelId);
      BaseModulePtr pModule = m_Slots[tunnel.m_nSlotId];
      if (pModule) {
        pModule->onSessionClosed(nTunnelId);
      }
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
      && sendToClient(m_Tunnels[nTunnelId].m_nFather, tunnelPDU);
}

void Commutator::closeSession(uint32_t nTunnelId)
{
  // in this context, sessionId (we got it from terminal) is a tunnelId
  if (nTunnelId >= m_Tunnels.size())
    return;

  Tunnel& tunnel = m_Tunnels[nTunnelId];
  if (!tunnel.m_lUp) {
    return;
  }

  spex::ICommutator message;
  message.set_close_tunnel_report(nTunnelId);
  sendToClient(tunnel.m_nFather, std::move(message));
  tunnel = Tunnel();
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
      case spex::ICommutator::kTotalSlotsReq:
        onGetTotalSlotsRequest(nSessionId);
        return;
      case spex::ICommutator::kModuleInfoReq:
        getModuleInfo(nSessionId, commutatorPdu.module_info_req());
        return;
      case spex::ICommutator::kAllModulesInfoReq:
        getAllModulesInfo(nSessionId);
        return;
      case spex::ICommutator::kOpenTunnel:
        onOpenTunnelRequest(nSessionId, commutatorPdu.open_tunnel());
        return;
      case spex::ICommutator::kCloseTunnel:
        onCloseTunnelRequest(nSessionId, commutatorPdu.close_tunnel());
        return;
      default:
        return;
    }
  }
}

void Commutator::onGetTotalSlotsRequest(uint32_t nSessionId) const
{
  spex::ICommutator message;
  message.set_total_slots(static_cast<uint32_t>(m_Slots.size()));
  sendToClient(nSessionId, std::move(message));
}

void Commutator::getModuleInfo(uint32_t nSessionId, uint32_t nSlotId) const
{
  spex::ICommutator message;
  message.mutable_module_info()->set_slot_id(nSlotId);
  if (nSlotId >= m_Slots.size() || !m_Slots[nSlotId]) {
    message.mutable_module_info()->set_module_type("empty");
  } else {
    message.mutable_module_info()->set_module_type(m_Slots[nSlotId]->getModuleType());
    message.mutable_module_info()->set_module_name(m_Slots[nSlotId]->getModuleName());
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
    message.mutable_module_info()->set_slot_id(nSlotId);
    message.mutable_module_info()->set_module_type(m_Slots[nSlotId]->getModuleType());
    message.mutable_module_info()->set_module_name(m_Slots[nSlotId]->getModuleName());
    sendToClient(nSessionId, std::move(message));
  }
}

void Commutator::onOpenTunnelRequest(uint32_t nSessionId, uint32_t nSlot)
{
  if (nSlot >= m_Slots.size() || !m_Slots[nSlot]) {
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::INVALID_SLOT);
    return;
  }

  BaseModulePtr pModule = m_Slots[nSlot];
  if (!pModule || !pModule->isOnline()) {
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::MODULE_OFFLINE);
    return;
  }

  uint32_t nTunnelId;
  if (!m_ReusableTunnels.empty()) {
    nTunnelId = m_ReusableTunnels.top();
    m_ReusableTunnels.pop();
  } else {
    m_Tunnels.push_back(Tunnel());
    nTunnelId = static_cast<uint32_t>(m_Tunnels.size() - 1);
  }

  if (!pModule->openSession(nTunnelId)) {
    m_ReusableTunnels.push(nTunnelId);
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::REJECTED_BY_MODULE);
    return;
  }

  Tunnel& tunnel      = m_Tunnels[nTunnelId];
  tunnel.m_nSlotId    = nSlot;
  tunnel.m_nFather = nSessionId;
  tunnel.m_lUp        = true;

  spex::ICommutator message;
  message.set_open_tunnel_report(nTunnelId);
  sendToClient(nSessionId, message);
}

void Commutator::onCloseTunnelRequest(uint32_t nSessionId, uint32_t nTunnelId)
{
  if (nTunnelId >= m_Tunnels.size()) {
    sendCloseTunnelFailed(nSessionId, spex::ICommutator::INVALID_TUNNEL);
    return;
  }

  Tunnel& tunnel = m_Tunnels[nTunnelId];
  if (!tunnel.m_lUp || tunnel.m_nFather != nSessionId) {
    sendCloseTunnelFailed(nSessionId, spex::ICommutator::INVALID_TUNNEL);
    return;
  }

  BaseModulePtr pModule = m_Slots[tunnel.m_nSlotId];
  if (pModule) {
    pModule->onSessionClosed(nTunnelId);
  }

  spex::ICommutator message;
  message.set_close_tunnel_report(nTunnelId);
  sendToClient(tunnel.m_nFather, std::move(message));
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
      message.set_close_tunnel_report(nTunnelId);
      sendToClient(m_Tunnels[nTunnelId].m_nFather, std::move(message));
    }
  }
}

void Commutator::onModuleHasBeenAttached(uint32_t nSlotId)
{
  spex::Message message;
  spex::ICommutator::ModuleInfo* pBody =
      message.mutable_commutator()->mutable_module_info();
  pBody->set_slot_id(nSlotId);
  pBody->set_module_type(m_Slots[nSlotId]->getModuleType());
  pBody->set_module_name(m_Slots[nSlotId]->getModuleName());
  broadcast(message);
}

void Commutator::sendOpenTunnelFailed(uint32_t nSessionId,
                                      spex::ICommutator::Status eReason)
{
  spex::ICommutator message;
  message.set_open_tunnel_failed(eReason);
  sendToClient(nSessionId, message);
}

void Commutator::sendCloseTunnelFailed(uint32_t nSessionId,
                                       spex::ICommutator::Status eReason)
{
  spex::ICommutator message;
  message.set_close_tunnel_failed(eReason);
  sendToClient(nSessionId, message);
}

} // namespace modules

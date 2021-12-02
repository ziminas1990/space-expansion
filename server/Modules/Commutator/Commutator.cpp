#include "Commutator.h"

#include <assert.h>
#include <Protocol.pb.h>

#include <SystemManager.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Commutator);

namespace modules
{

Commutator::Commutator()
  : BaseModule("Commutator", std::string(), world::PlayerWeakPtr())
{
  GlobalObject<Commutator>::registerSelf(this);
  // Slot #0 should not be used. It make
  m_Tunnels.emplace_back();
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
  for (uint32_t nTunnelId = 1; nTunnelId < m_Tunnels.size(); ++nTunnelId)
  {
    Tunnel& tunnel = m_Tunnels[nTunnelId];
    m_Slots[tunnel.m_nSlotId]->onSessionClosed(tunnel.m_nParentSessionId);
    tunnel.m_lUp = false;
  }
  m_Tunnels.clear();
  for (BaseModulePtr& pModule : m_Slots) {
    pModule->detachFromChannel();
  }
}

void Commutator::checkSlotsAndTunnels()
{
  for (uint32_t nTunnelId = 1; nTunnelId < m_Tunnels.size(); ++nTunnelId)
  {
    Tunnel& tunnelRef = m_Tunnels[nTunnelId];
    if (!tunnelRef.m_lUp)
      continue;
    BaseModulePtr const& pModule = m_Slots[tunnelRef.m_nSlotId];
    if (!pModule || !pModule->isOnline()) {
      sendCloseTunnelInd(nTunnelId);
      tunnelRef = Tunnel();
      m_ReusableTunnels.push(nTunnelId);
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

void Commutator::proceed(uint32_t)
{
  uint64_t nNow = SystemManager::getIngameTime();
  for(size_t i = 0; i < m_delayedMessages.size(); ++i) {
    StoredMessage& message = m_delayedMessages[i];
    if(message.m_message.timestamp() < nNow) {
      commutateMessage(message.m_nTunnelId, message.m_message);
      // Removing this message from array (swap with last element and pop last element):
      if (i + 1 < m_delayedMessages.size()) {
        std::swap(m_delayedMessages[i], m_delayedMessages.back());
      }
      m_delayedMessages.pop_back();
    }
  }
  if (m_delayedMessages.empty()) {
    switchToIdleState();
  }
}

void Commutator::onMessageReceived(uint32_t nSessionId, spex::Message const& message)
{
  if (message.choice_case() == spex::Message::kEncapsulated) {
    // This exception is done to prevent loosing time for tunneling
    commutateMessage(message.tunnelid(), message.encapsulated());
  } else {
    BaseModule::onMessageReceived(nSessionId, message);
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
  for (uint32_t nTunnelId = 1; nTunnelId <= m_Tunnels.size(); ++nTunnelId)
  {
    Tunnel& tunnelRef = m_Tunnels[nTunnelId];
    if (tunnelRef.m_nParentSessionId == nSessionId) {
      BaseModulePtr pModule = m_Slots[tunnelRef.m_nSlotId];
      if (pModule) {
        pModule->onSessionClosed(nTunnelId);
      }
      sendCloseTunnelInd(nTunnelId);
      tunnelRef = Tunnel();
      m_ReusableTunnels.push(nTunnelId);
    }
  }
}

bool Commutator::send(uint32_t nTunnelId, spex::Message const& message) const
{
  // in this context, sessionId (we got it from terminal) is a tunnelId
  assert(nTunnelId > 0);
  spex::Message tunnelPDU;
  tunnelPDU.set_tunnelid(nTunnelId);

  spex::Message* encapsulated = tunnelPDU.mutable_encapsulated();
  *encapsulated = message;
  if (encapsulated->choice_case() != spex::Message::kEncapsulated) {
    // If we are the top-level commutator for this message, then we should add
    // timestamp to it
    encapsulated->set_timestamp(SystemManager::getIngameTime());
  }
  if (nTunnelId < m_Tunnels.size()) {
    const Tunnel& tunnel = m_Tunnels[nTunnelId];
    return tunnel.m_lUp
        && sendToClient(tunnel.m_nParentSessionId, tunnelPDU);
  }
  return false;
}

void Commutator::closeSession(uint32_t nTunnelId)
{
  // in this context, sessionId (we got it from terminal) is a tunnelId
  if (nTunnelId >= m_Tunnels.size() || nTunnelId == 0)
    return;

  Tunnel& tunnelRef = m_Tunnels[nTunnelId];
  if (!tunnelRef.m_lUp) {
    return;
  }
  sendCloseTunnelInd(nTunnelId);
  tunnelRef = Tunnel();
  m_ReusableTunnels.push(nTunnelId);
}

void Commutator::detachFromTerminal()
{
  detachFromModules();
}

void Commutator::handleMessage(uint32_t nSessionId, spex::Message const& message)
{
  // This case must be handled in 'Commutator::onMessageReceived'
  assert(message.choice_case() != spex::Message::kEncapsulated);

  if (message.choice_case() != spex::Message::kCommutator) {
    return;
  }
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

void Commutator::onGetTotalSlotsRequest(uint32_t nSessionId) const
{
  spex::Message message;
  message.mutable_commutator()->set_total_slots(static_cast<uint32_t>(m_Slots.size()));
  sendToClient(nSessionId, message);
}

void Commutator::getModuleInfo(uint32_t nSessionId, uint32_t nSlotId) const
{
  spex::Message response;
  spex::ICommutator::ModuleInfo* pBody =
      response.mutable_commutator()->mutable_module_info();
  pBody->set_slot_id(nSlotId);
  if (nSlotId >= m_Slots.size() || !m_Slots[nSlotId]) {
    pBody->set_module_type("empty");
  } else {
    pBody->set_module_type(m_Slots[nSlotId]->getModuleType());
    pBody->set_module_name(m_Slots[nSlotId]->getModuleName());
  }
  sendToClient(nSessionId, response);
}

void Commutator::getAllModulesInfo(uint32_t nSessionId) const
{
  for (uint32_t nSlotId = 0; nSlotId < m_Slots.size(); ++nSlotId)
  {
    if (!m_Slots[nSlotId])
      continue;
    spex::Message response;
    spex::ICommutator::ModuleInfo* pBody =
        response.mutable_commutator()->mutable_module_info();
    pBody->set_slot_id(nSlotId);
    pBody->set_module_type(m_Slots[nSlotId]->getModuleType());
    pBody->set_module_name(m_Slots[nSlotId]->getModuleName());
    sendToClient(nSessionId, response);
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

  Tunnel& tunnel            = m_Tunnels[nTunnelId];
  tunnel.m_nSlotId          = nSlot;
  tunnel.m_nParentSessionId = nSessionId;
  tunnel.m_lUp              = true;

  spex::Message message;
  message.mutable_commutator()->set_open_tunnel_report(nTunnelId);
  sendToClient(nSessionId, message);
}

void Commutator::onCloseTunnelRequest(uint32_t nSessionId, uint32_t nTunnelId)
{
  if (nTunnelId >= m_Tunnels.size() || nTunnelId == 0) {
    sendCloseTunnelStatus(nSessionId, spex::ICommutator::INVALID_TUNNEL);
    return;
  }

  Tunnel& tunnelRef = m_Tunnels[nTunnelId];
  if (!tunnelRef.m_lUp) {
    sendCloseTunnelStatus(nSessionId, spex::ICommutator::INVALID_TUNNEL);
    return;
  }

  BaseModulePtr pModule = m_Slots[tunnelRef.m_nSlotId];
  if (pModule) {
    pModule->onSessionClosed(nTunnelId);
  }

  sendCloseTunnelStatus(nSessionId, spex::ICommutator::SUCCESS);
  sendCloseTunnelInd(nTunnelId);
  tunnelRef = Tunnel();
  m_ReusableTunnels.push(nTunnelId);
}

void Commutator::commutateMessage(
    uint32_t nTunnelId,
    spex::Message const& message)
{
  if (message.timestamp() &&
      message.timestamp() > SystemManager::getIngameTime()) {
    m_delayedMessages.emplace_back(nTunnelId, message);
    switchToActiveState();
    return;
  }

  if (nTunnelId < m_Tunnels.size() && nTunnelId != 0) {
    Tunnel& tunnel = m_Tunnels[nTunnelId];
    if (tunnel.m_lUp && tunnel.m_nSlotId < m_Slots.size()) {
      BaseModulePtr& pModule = m_Slots[tunnel.m_nSlotId];
      if (pModule && pModule->isOnline()) {
        pModule->onMessageReceived(nTunnelId, message);
        return;
      }
    }
  }
  // Seems that tunnel is closed or doesn't exist
  sendCloseTunnelInd(nTunnelId);
}

void Commutator::onModuleHasBeenDetached(uint32_t nSlotId)
{
  m_Slots[nSlotId] = nullptr;
  // We assume, that this operation is rather rare therefore we can afford to
  // execute it in O(N) time
  for (uint32_t nTunnelId = 1; nTunnelId < m_Tunnels.size(); ++nTunnelId) {
    if (m_Tunnels[nTunnelId].m_nSlotId == nSlotId) {
      sendCloseTunnelInd(nTunnelId);
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
  spex::Message message;
  message.mutable_commutator()->set_open_tunnel_failed(eReason);
  sendToClient(nSessionId, message);
}

void Commutator::sendCloseTunnelStatus(uint32_t nSessionId,
                                       spex::ICommutator::Status eStatus)
{
  spex::Message message;
  message.mutable_commutator()->set_close_tunnel_status(eStatus);
  sendToClient(nSessionId, message);
}

void Commutator::sendCloseTunnelInd(uint32_t nTunnelId)
{
  // Indication must be sent into tunnel, not to the parent session
  spex::Message message;
  message.mutable_commutator()->set_close_tunnel_ind(true);
  send(nTunnelId, message);
}

} // namespace modules

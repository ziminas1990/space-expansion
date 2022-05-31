#include "Commutator.h"

#include <assert.h>
#include <Protocol.pb.h>

#include <Utils/Clock.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Commutator);

namespace modules
{

bool Commutator::Slot::removeSessionId(uint32_t nSessionId)
{
  const size_t total = m_activeSessions.size();
  for (size_t i = 0; i < total; ++i) {
    if (m_activeSessions[i] == nSessionId) {
      m_activeSessions[i] = m_activeSessions.back();
      m_activeSessions.pop_back();
      return true;
    }
  }
  return false;
}

void Commutator::Slot::reset()
{
  m_pModule = nullptr;
  m_activeSessions.clear();
  assert(!isValid());
}

Commutator::Commutator(std::shared_ptr<network::SessionMux> pSessionMux)
  : BaseModule("Commutator", std::string(), world::PlayerWeakPtr())
  , m_pSessionMux(pSessionMux)
{
  GlobalObject<Commutator>::registerSelf(this);
}

uint32_t Commutator::attachModule(BaseModulePtr pModule)
{
  // We assume, that this operation is rather rare therefore we can afford to
  // execute it in O(N) time
  pModule->attachToChannel(m_pSessionMux->asChannel());
  
  for (uint32_t nSlotId = 0; nSlotId < m_slots.size(); ++nSlotId)
  {
    if (m_slots[nSlotId].m_pModule->isDestroyed())
    {
      onModuleHasBeenDetached(nSlotId);
      m_slots[nSlotId].m_pModule = pModule;
      return nSlotId;
    }
  }
  m_slots.push_back({pModule, std::vector<uint32_t>()});
  return static_cast<uint32_t>(m_slots.size()) - 1;
}

BaseModulePtr Commutator::findModuleByName(std::string const& sName) const
{
  for (const Slot& slot : m_slots) {
    if (slot.m_pModule->getModuleName() == sName)
      return slot.m_pModule;
  }
  return BaseModulePtr();
}

BaseModulePtr Commutator::findModuleByType(std::string const& sType) const
{
  for (const Slot& slot : m_slots) {
    if (slot.m_pModule->getModuleType() == sType)
      return slot.m_pModule;
  }
  return BaseModulePtr();
}

void Commutator::detachFromModules()
{
  for (Slot& slot: m_slots) {
    for (uint32_t nSessionId : slot.m_activeSessions) {
      m_pSessionMux->closeSession(nSessionId);
      slot.m_pModule->onSessionClosed(nSessionId);
    }
    slot.m_pModule->detachFromChannel();
    slot.reset();
  }
  m_slots.clear();
}

void Commutator::checkSlots()
{
  for (Slot& slot: m_slots) {
    if (slot.m_pModule) {
      if (!slot.m_pModule->isOnline()) {
        for (uint32_t nSessionId : slot.m_activeSessions) {
          m_pSessionMux->closeSession(nSessionId);
        }
        slot.m_activeSessions.clear();
      }
      if (slot.m_pModule->isDestroyed()) {
        slot.m_pModule = nullptr;
      }
    }
  }
}

void Commutator::handleMessage(uint32_t nSessionId, spex::Message const& message)
{
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
  message.mutable_commutator()->set_total_slots(
    static_cast<uint32_t>(m_slots.size()));
  sendToClient(nSessionId, message);
}

void Commutator::getModuleInfo(uint32_t nSessionId, uint32_t nSlotId) const
{
  spex::Message response;
  spex::ICommutator::ModuleInfo* pBody =
      response.mutable_commutator()->mutable_module_info();
  pBody->set_slot_id(nSlotId);
  if (nSlotId >= m_slots.size() || !m_slots[nSlotId].isValid()) {
    pBody->set_module_type("empty");
  } else {
    const BaseModulePtr pModule = m_slots[nSlotId].m_pModule;
    pBody->set_module_type(pModule->getModuleType());
    pBody->set_module_name(pModule->getModuleName());
  }
  sendToClient(nSessionId, response);
}

void Commutator::getAllModulesInfo(uint32_t nSessionId) const
{
  const size_t nTotalSlots = m_slots.size();
  for (uint32_t nSlotId = 0; nSlotId < nTotalSlots; ++nSlotId)
  {
    if (m_slots[nSlotId].m_pModule) {
      spex::Message response;
      spex::ICommutator::ModuleInfo* pBody =
          response.mutable_commutator()->mutable_module_info();
      pBody->set_slot_id(nSlotId);
      const BaseModulePtr pModule = m_slots[nSlotId].m_pModule;
      pBody->set_module_type(pModule->getModuleType());
      pBody->set_module_name(pModule->getModuleName());
      sendToClient(nSessionId, response);
    }
  }
}

void Commutator::onOpenTunnelRequest(uint32_t nSessionId, uint32_t nSlot)
{
  if (nSlot >= m_slots.size()) {
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::INVALID_SLOT);
    return;
  }

  BaseModulePtr pModule = m_slots[nSlot].m_pModule;
  if (!pModule || !pModule->isOnline()) {
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::MODULE_OFFLINE);
    return;
  }

  const uint32_t nChildSessionId = 
      m_pSessionMux->createSession(nSessionId, pModule);

  if (!pModule->openSession(nChildSessionId)) {
    m_pSessionMux->closeSession(nChildSessionId);
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::REJECTED_BY_MODULE);
    return;
  }
  m_slots[nSlot].m_activeSessions.push_back(nChildSessionId);

  spex::Message message;
  message.mutable_commutator()->set_open_tunnel_report(nChildSessionId);
  sendToClient(nSessionId, message);
}

void Commutator::onCloseTunnelRequest(uint32_t nSessionId, uint32_t nTunnelId)
{
  if (m_pSessionMux->closeSession(nTunnelId)) {
    sendCloseTunnelStatus(nSessionId, spex::ICommutator::INVALID_TUNNEL);
    return;
  }

  // Linear search here :(
  for (Slot& slot: m_slots) {
    if (slot.removeSessionId(nSessionId)) {
      break;
    }
  }

  sendCloseTunnelStatus(nSessionId, spex::ICommutator::SUCCESS);
}

void Commutator::onModuleHasBeenDetached(uint32_t nSlotId)
{
  Slot& slot = m_slots[nSlotId];
  for (uint32_t nSessionId : slot.m_activeSessions) {
    m_pSessionMux->closeSession(nSessionId);
    slot.m_pModule->onSessionClosed(nSessionId);
  }
  slot.reset();
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

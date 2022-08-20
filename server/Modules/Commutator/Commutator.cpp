#include "Commutator.h"

#include <assert.h>
#include <Protocol.pb.h>

#include <Network/SessionMux.h>
#include <Utils/Clock.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::Commutator);

namespace modules
{

Commutator::Commutator(network::SessionMuxWeakPtr pSessionMux)
  : BaseModule("Commutator", std::string(), world::PlayerWeakPtr())
  , m_pSessionMux(pSessionMux)
{
  GlobalObject<Commutator>::registerSelf(this);
  m_monitoringSessions.reserve(16);
}

uint32_t Commutator::attachModule(BaseModulePtr pModule)
{
  // We assume, that this operation is rather rare therefore we can afford to
  // execute it in O(N) time
  network::SessionMuxPtr pSessionMux = m_pSessionMux.lock();
  assert(pSessionMux);

  pModule->attachToChannel(pSessionMux->asChannel());
  
  for (uint32_t nSlotId = 0; nSlotId < m_modules.size(); ++nSlotId)
  {
    if (m_modules[nSlotId]->isDestroyed())
    {
      onModuleHasBeenDetached(nSlotId);
      m_modules[nSlotId] = pModule;
      sendModuleAttachedUpdate(nSlotId);
      return nSlotId;
    }
  }
  m_modules.push_back(pModule);
  const uint32_t nSlotId = static_cast<uint32_t>(m_modules.size()) - 1;
  sendModuleAttachedUpdate(nSlotId);
  return nSlotId;
}

bool Commutator::detachModule(uint32_t nSlotId, const BaseModulePtr& pModule)
{
  if (nSlotId >= m_modules.size()) {
    assert(!"Invalid slot!");
    return false;
  }

  const BaseModulePtr& pInstalledModule = m_modules[nSlotId];
  if (pInstalledModule != pModule) {
    assert(!"Unexpected module");
    return false;
  }
  pModule->detachFromChannel();

  network::SessionMuxPtr pSessionMux = m_pSessionMux.lock();
  if (pSessionMux) {
    for (const uint32_t nSessionId : pModule->getOpenedSession()) {
      pSessionMux->closeSession(nSessionId);
    }
  }
  m_modules[nSlotId].reset();
  sendModuleDetachedUpdate(nSlotId);
  return true;
}

BaseModulePtr Commutator::findModuleByName(std::string const& sName) const
{
  for (const BaseModulePtr& pModule : m_modules) {
    if (pModule->getModuleName() == sName)
      return pModule;
  }
  return BaseModulePtr();
}

BaseModulePtr Commutator::findModuleByType(std::string const& sType) const
{
  for (const BaseModulePtr& pModule: m_modules) {
    if (pModule->getModuleType() == sType)
      return pModule;
  }
  return BaseModulePtr();
}

void Commutator::checkSlots()
{
  // Check if any of modules, attached to commutator, have been destroyed

  network::SessionMuxPtr pSessionMux = m_pSessionMux.lock();
  assert(pSessionMux);

  const size_t nTotalModules = m_modules.size();
  for (uint32_t nSlotId = 0; nSlotId < nTotalModules; ++nSlotId) {
    BaseModulePtr& pModule = m_modules[nSlotId];
    if (pModule) {
      if (pModule->isOnline() || !pModule->hasOpenedSessions()) {  // [[likely]]
        continue;
      }
      // Module is offline or destroyed AND has opened sessions. Sessions
      // should be closed now.
      // Note: have to make a copy of opened sessions vector
      const std::vector<uint32_t> openedSessions = pModule->getOpenedSession();
      for (uint32_t nSessionId : openedSessions) {
        pSessionMux->closeSession(nSessionId);
      }
      if (pModule->isDestroyed()) {
        detachModule(nSlotId, pModule);
        // NOTE: 'pModule' reference is invalidated here
      }
    }
  }
}

void Commutator::onSessionClosed(uint32_t nSessionId)
{
  const size_t total = m_monitoringSessions.size();
  for (size_t i = 0; i < total; ++i) {
    if (m_monitoringSessions[i] == nSessionId) {
      m_monitoringSessions[i] = m_monitoringSessions.back();
      m_monitoringSessions.pop_back();
    }
  }
  BaseModule::onSessionClosed(nSessionId);
}

void Commutator::handleCommutatorMessage(uint32_t nSessionId,
                                         spex::ICommutator const& message)
{
  switch(message.choice_case()) {
    case spex::ICommutator::kTotalSlotsReq:
      onGetTotalSlotsRequest(nSessionId);
      return;
    case spex::ICommutator::kModuleInfoReq:
      getModuleInfo(nSessionId, message.module_info_req());
      return;
    case spex::ICommutator::kAllModulesInfoReq:
      getAllModulesInfo(nSessionId);
      return;
    case spex::ICommutator::kOpenTunnel:
      onOpenTunnelRequest(nSessionId, message.open_tunnel());
      return;
    case spex::ICommutator::kCloseTunnel:
      onCloseTunnelRequest(nSessionId, message.close_tunnel());
      return;
    case spex::ICommutator::kMonitor:
      onMonitoringRequest(nSessionId);
      return;
    default:
      return;
  }
}

void Commutator::onGetTotalSlotsRequest(uint32_t nSessionId) const
{
  spex::Message message;
  message.mutable_commutator()->set_total_slots(
    static_cast<uint32_t>(m_modules.size()));
  sendToClient(nSessionId, std::move(message));
}

void Commutator::getModuleInfo(uint32_t nSessionId, uint32_t nSlotId) const
{
  spex::Message response;
  spex::ICommutator::ModuleInfo* pBody =
      response.mutable_commutator()->mutable_module_info();
  pBody->set_slot_id(nSlotId);
  if (nSlotId >= m_modules.size() || !m_modules[nSlotId]) {
    pBody->set_module_type("empty");
  } else {
    const BaseModulePtr pModule = m_modules[nSlotId];
    pBody->set_module_type(pModule->getModuleType());
    pBody->set_module_name(pModule->getModuleName());
  }
  sendToClient(nSessionId, std::move(response));
}

void Commutator::getAllModulesInfo(uint32_t nSessionId) const
{
  const size_t nTotalModules = m_modules.size();
  for (uint32_t nSlotId = 0; nSlotId < nTotalModules; ++nSlotId) {
    const BaseModulePtr& pModule = m_modules[nSlotId];
    if (pModule) {
      spex::Message response;
      spex::ICommutator::ModuleInfo* pBody =
          response.mutable_commutator()->mutable_module_info();
      pBody->set_slot_id(nSlotId);
      pBody->set_module_type(pModule->getModuleType());
      pBody->set_module_name(pModule->getModuleName());
      sendToClient(nSessionId, std::move(response));
    }
  }
}

void Commutator::onOpenTunnelRequest(uint32_t nSessionId, uint32_t nSlot)
{
  network::SessionMuxPtr pSessionMux = m_pSessionMux.lock();
  if (!pSessionMux) {
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::COMMUTATOR_OFFLINE);
    return;
  }

  if (nSlot >= m_modules.size()) {
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::INVALID_SLOT);
    return;
  }

  BaseModulePtr pModule = m_modules[nSlot];
  if (!pModule || !pModule->isOnline()) {
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::MODULE_OFFLINE);
    return;
  }

  const uint32_t nChildSessionId = 
      pSessionMux->createSession(nSessionId, pModule);

  if (!pModule->openSession(nChildSessionId)) {
    pSessionMux->closeSession(nChildSessionId);
    sendOpenTunnelFailed(nSessionId, spex::ICommutator::REJECTED_BY_MODULE);
    return;
  }

  spex::Message message;
  message.mutable_commutator()->set_open_tunnel_report(nChildSessionId);
  sendToClient(nSessionId, std::move(message));
}

void Commutator::onCloseTunnelRequest(uint32_t nSessionId, uint32_t nTunnelId)
{
  network::SessionMuxPtr pSessionMux = m_pSessionMux.lock();
  if (!pSessionMux) {
    sendCloseTunnelStatus(nSessionId, spex::ICommutator::COMMUTATOR_OFFLINE);
    return;
  }

  if (!pSessionMux->closeSession(nTunnelId) && nTunnelId == nSessionId) {
    sendCloseTunnelStatus(nSessionId, spex::ICommutator::INVALID_TUNNEL);
    return;
  }

  sendCloseTunnelStatus(nSessionId, spex::ICommutator::SUCCESS);
}

void Commutator::onMonitoringRequest(uint32_t nSessionId)
{
  if (m_monitoringSessions.size() == 8) {
    sendMonitorStatus(nSessionId, spex::ICommutator::TOO_MANY_SESSIONS);
    return;
  }
  m_monitoringSessions.push_back(nSessionId);
  sendMonitorStatus(nSessionId, spex::ICommutator::SUCCESS);
}

void Commutator::onModuleHasBeenDetached(uint32_t nSlotId)
{
  network::SessionMuxPtr pSessionMux = m_pSessionMux.lock();
  assert(pSessionMux);

  if (nSlotId >= m_modules.size()) {
    assert(!"Inconsistent state!");
    return;
  }

  BaseModulePtr& pModule = m_modules[nSlotId];
  if (pModule->hasOpenedSessions()) {
    // Have to make a copy of opened sessions vector
    std::vector<uint32_t> openedSessions = pModule->getOpenedSession();
    for (const uint32_t nSessionId : openedSessions) {
      pSessionMux->closeSession(nSessionId);
    }
  }
  pModule.reset();
}

void Commutator::sendOpenTunnelFailed(uint32_t nSessionId,
                                      spex::ICommutator::Status eReason)
{
  spex::Message message;
  message.mutable_commutator()->set_open_tunnel_failed(eReason);
  sendToClient(nSessionId, std::move(message));
}

void Commutator::sendCloseTunnelStatus(uint32_t nSessionId,
                                       spex::ICommutator::Status eStatus)
{
  spex::Message message;
  message.mutable_commutator()->set_close_tunnel_status(eStatus);
  sendToClient(nSessionId, std::move(message));
}

void Commutator::sendMonitorStatus(uint32_t nSessionId,
                                   spex::ICommutator::Status eStatus) const
{
  spex::Message message;
  message.mutable_commutator()->set_monitor_ack(eStatus);
  sendToClient(nSessionId, std::move(message));
}

void Commutator::sendModuleAttachedUpdate(uint32_t nSlotId) const
{
  assert(nSlotId < m_modules.size() && m_modules[nSlotId]);
  const BaseModulePtr& pModule = m_modules[nSlotId];

  spex::Message update;
  spex::ICommutator::ModuleInfo* pBody =
      update.mutable_commutator()->mutable_update()->mutable_module_attached();
  pBody->set_slot_id(nSlotId);
  pBody->set_module_type(pModule->getModuleType());
  pBody->set_module_name(pModule->getModuleName());

  for (uint32_t nMonitoringSession: m_monitoringSessions) {
    sendToClient(nMonitoringSession, spex::Message(update));
  }
}

void Commutator::sendModuleDetachedUpdate(uint32_t nSlotId) const
{
  spex::Message update;
  update.mutable_commutator()->mutable_update()->set_module_detached(nSlotId);
  for (uint32_t nMonitoringSession: m_monitoringSessions) {
    sendToClient(nMonitoringSession, spex::Message(update));
  }
}

} // namespace modules

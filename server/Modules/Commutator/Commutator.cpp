#include "Commutator.h"

#include <assert.h>
#include <Protocol.pb.h>

#include <Network/SessionMux.h>
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

Commutator::Commutator(network::SessionMuxWeakPtr pSessionMux)
  : BaseModule("Commutator", std::string(), world::PlayerWeakPtr())
  , m_pSessionMux(pSessionMux)
{
  GlobalObject<Commutator>::registerSelf(this);
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
      return nSlotId;
    }
  }
  m_modules.push_back(pModule);
  m_activeSessions.push_back({});
  return static_cast<uint32_t>(m_modules.size()) - 1;
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
    for (uint32_t nSessionId : m_activeSessions[nSlotId]) {
      pSessionMux->closeSession(nSessionId);
    }
  }
  m_activeSessions[nSlotId].clear();
  m_modules[nSlotId].reset();
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

void Commutator::detachFromModules()
{
  network::SessionMuxPtr pSessionMux = m_pSessionMux.lock();
  assert(pSessionMux);

  const size_t nTotalSolts = m_modules.size();
  assert(m_modules.size() == m_activeSessions.size());
  for (uint32_t nSlotId = 0; nSlotId < nTotalSolts; ++nSlotId) {
    for (uint32_t nSessionId : m_activeSessions[nSlotId]) {
      pSessionMux->closeSession(nSessionId);
    }
  }
  m_modules.clear();
  m_activeSessions.clear();
}

void Commutator::checkSlots()
{
  network::SessionMuxPtr pSessionMux = m_pSessionMux.lock();
  assert(pSessionMux);

  const size_t nTotalModules = m_modules.size();
  assert(m_activeSessions.size() == nTotalModules);
  for (uint32_t nSlotId = 0; nSlotId < nTotalModules; ++nSlotId) {
    BaseModulePtr& pModule                = m_modules[nSlotId];
    std::vector<uint32_t>& activeSessions = m_activeSessions[nSlotId];

    if (pModule) {
      if (!pModule->isOnline()) {
        for (uint32_t nSessionId : activeSessions) {
          pSessionMux->closeSession(nSessionId);
        }
        activeSessions.clear();
      }
      if (pModule->isDestroyed()) {
        pModule.reset();
      }
    }
  }
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
  m_activeSessions[nSlot].push_back(nChildSessionId);

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

  if (!pSessionMux->closeSession(nTunnelId)) {
    sendCloseTunnelStatus(nSessionId, spex::ICommutator::INVALID_TUNNEL);
    return;
  }

  // Linear search here :(
  for (std::vector<uint32_t>& sessions: m_activeSessions) {
    const size_t total = sessions.size();
    for (size_t i = 0; i < total; ++i) {
      if (sessions[i] == nSessionId) {
        sessions[i] = sessions.back();
        sessions.pop_back();
        return;
      }
    }
  }

  sendCloseTunnelStatus(nSessionId, spex::ICommutator::SUCCESS);
}

void Commutator::onModuleHasBeenDetached(uint32_t nSlotId)
{
  network::SessionMuxPtr pSessionMux = m_pSessionMux.lock();
  assert(pSessionMux);

  if (nSlotId >= m_modules.size()) {
    assert(!"Inconsistent state!");
    return;
  }

  for (uint32_t nSessionId : m_activeSessions[nSlotId]) {
    pSessionMux->closeSession(nSessionId);
    m_modules[nSlotId]->onSessionClosed(nSessionId);
  }
  m_modules[nSlotId].reset();
  m_activeSessions[nSlotId].clear();
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

void Commutator::sendCloseTunnelInd(uint32_t nTunnelId)
{
  // Indication must be sent into tunnel, not to the parent session
  spex::Message message;
  message.mutable_commutator()->set_close_tunnel_ind(true);
  send(nTunnelId, std::move(message));
}

} // namespace modules

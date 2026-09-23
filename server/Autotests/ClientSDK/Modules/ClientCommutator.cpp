#include "ClientCommutator.h"
#include <Protocol.pb.h>

namespace autotests { namespace client {

bool ClientCommutator::getTotalSlots(uint32_t& nTotalSlots)
{
  return sendTotalSlotsReq() && waitTotalSlots(nTotalSlots);
}

bool ClientCommutator::getModuleInfo(uint32_t nSlotId, ModuleInfo& info)
{
  spex::Message request;
  request.mutable_commutator()->set_module_info_req(nSlotId);
  if (!send(std::move(request)))
    return false;

  spex::ICommutator response;
  if (!wait(response))
    return false;
  if (response.choice_case() != spex::ICommutator::kModuleInfo)
    return false;
  info = ModuleInfo({response.module_info().slot_id(),
                     response.module_info().module_type(),
                     response.module_info().module_name(),
                     response.module_info().blueprint_name()});
  return true;
}

bool ClientCommutator::getAttachedModulesList(ModulesList& attachedModules)
{
  spex::Message request;
  request.mutable_commutator()->set_all_modules_info_req(true);
  if (!send(std::move(request))) {
    return false;
  }

  spex::ICommutator response;
  if (!wait(response)) {
    return false;
  }

  if (response.choice_case() != spex::ICommutator::kModulesInfoList) {
    return false;
  }

  const auto& modules = response.modules_info_list().modules();
  for (const auto& module : modules) {
    attachedModules.push_back(
          ModuleInfo({module.slot_id(),
                      module.module_type(),
                      module.module_name(),
                      module.blueprint_name()}));
  }
  return true;
}

Router::SessionPtr ClientCommutator::openSession(uint32_t nSlotId)
{
  if (!sendOpenTunnel(nSlotId))
    return Router::SessionPtr();

  uint32_t nSessionId = 0;
  if (!waitOpenTunnelSuccess(&nSessionId))
    return Router::SessionPtr();

  return m_pRouter->openSession(nSessionId);
}

bool ClientCommutator::closeTunnel(Router::SessionPtr pSession)
{
  if (!sendCloseTunnel(pSession->sessionId())) {
    return false;
  }

  spex::ICommutator::Status status;
  if (!waitCloseTunnelStatus(status)) {
    return false;
  }

  return status == spex::ICommutator::SUCCESS
      && !m_pRouter->hasSession(pSession->sessionId());
}

bool ClientCommutator::monitoring()
{
  spex::Message request;
  request.mutable_commutator()->set_monitor(true);
  return send(std::move(request))
      && waitMonitoringStatus(spex::ICommutator::SUCCESS);
}

bool ClientCommutator::waitMonitoringStatus(spex::ICommutator::Status expected)
{
  spex::ICommutator message;
  return wait(message)
      && message.choice_case() == spex::ICommutator::kMonitorAck
      && message.monitor_ack() == expected;
}

bool ClientCommutator::waitUpdate(spex::ICommutator::Update& update)
{
  spex::ICommutator message;
  if (!wait(message) || message.choice_case() != spex::ICommutator::kUpdate) {
    return false;
  }
  update = message.update();
  return true;
}

bool ClientCommutator::waitModuleAttached(spex::ICommutator::ModuleInfo& info)
{
  spex::ICommutator::Update update;
  if (!waitUpdate(update) ||
      update.choice_case() != spex::ICommutator::Update::kModuleAttached) {
    return false;
  }
  info = update.module_attached();
  return true;
}

bool ClientCommutator::waitModuleDetached(uint32_t& nSlotId)
{
  spex::ICommutator::Update update;
  if (!waitUpdate(update) ||
      update.choice_case() != spex::ICommutator::Update::kModuleDetached) {
    return false;
  }
  nSlotId = update.module_detached();
  return true;
}

bool ClientCommutator::sendOpenTunnel(uint32_t nSlotId)
{
  spex::Message request;
  request.mutable_commutator()->set_open_tunnel(nSlotId);
  return send(std::move(request));
}

bool ClientCommutator::waitOpenTunnelSuccess(uint32_t *pOpenedTunnelId)
{
  spex::ICommutator message;
  if (!wait(message))
    return false;
  if (message.choice_case() == spex::ICommutator::kOpenTunnelReport) {
    if (pOpenedTunnelId) {
      *pOpenedTunnelId = message.open_tunnel_report();
    }
    return true;
  }
  assert(message.choice_case() == spex::ICommutator::kOpenTunnelFailed);
  return false;
}

bool ClientCommutator::waitOpenTunnelFailed()
{
  spex::ICommutator message;
  return wait(message)
      && message.choice_case() == spex::ICommutator::kOpenTunnelFailed;
}

bool ClientCommutator::sendCloseTunnel(uint32_t nTunnelId)
{
  spex::Message request;
  request.mutable_commutator()->set_close_tunnel(nTunnelId);
  return send(std::move(request));
}

bool ClientCommutator::waitCloseTunnelStatus(spex::ICommutator::Status& status)
{
  spex::ICommutator message;
  if (!wait(message)) {
    return false;
  }
  if (message.choice_case() != spex::ICommutator::kCloseTunnelStatus) {
    return false;
  }
  status = message.close_tunnel_status();
  return true;
}

bool ClientCommutator::sendTotalSlotsReq()
{
  spex::Message request;
  request.mutable_commutator()->set_total_slots_req(true);
  return send(std::move(request));
}

bool ClientCommutator::waitTotalSlots(uint32_t& nSlots)
{
  spex::ICommutator message;
  if (!wait(message))
    return false;
  if (message.choice_case() != spex::ICommutator::kTotalSlots)
    return false;
  nSlots = message.total_slots();
  return true;
}

}}  // namespace autotests::client

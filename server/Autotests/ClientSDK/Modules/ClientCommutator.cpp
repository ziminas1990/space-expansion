#include "ClientCommutator.h"
#include <Protocol.pb.h>

namespace autotests { namespace client {

bool ClientCommutator::getTotalSlots(uint32_t& nTotalSlots)
{
  return sendTotalSlotsReq() && waitTotalSlots(nTotalSlots);
}

bool ClientCommutator::getAttachedModulesList(ModulesList& attachedModules)
{
  uint32_t nTotal = 0;
  if (!getTotalSlots(nTotal))
    return false;

  spex::Message request;
  request.mutable_commutator()->set_all_modules_info_req(true);
  if (!send(std::move(request)))
    return false;

  for (size_t i = 0; i < nTotal; ++i) {
    spex::ICommutator response;
    if (!wait(response))
      return false;
    if (response.choice_case() != spex::ICommutator::kModuleInfo)
      return false;
    attachedModules.push_back(
          ModuleInfo({response.module_info().slot_id(),
                      response.module_info().module_type(),
                      response.module_info().module_name()}));
  }
  return attachedModules.size() == nTotal;
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
      && m_pRouter->closeSession(pSession->sessionId());
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

bool ClientCommutator::waitGameOverReport(spex::IGame::GameOver& report,
                                          uint16_t nTimeout)
{
  spex::IGame message;
  if (!wait(message, nTimeout)) {
    return false;
  }

  if (message.choice_case() != spex::IGame::kGameOverReport) {
    return false;
  }

  report = std::move(message.game_over_report());
  return true;
}

}}  // namespace autotests::client

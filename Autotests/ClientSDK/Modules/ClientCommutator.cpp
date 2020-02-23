#include "ClientCommutator.h"
#include <Protocol.pb.h>

namespace autotests { namespace client {

bool ClientCommutator::getTotalSlots(uint32_t& nTotalSlots)
{
  spex::Message request;
  request.mutable_commutator()->set_total_slots_req(true);
  if (!send(request))
    return false;

  spex::ICommutator message;
  if (!wait(message))
    return false;
  if (message.choice_case() != spex::ICommutator::kTotalSlots)
    return false;
  nTotalSlots = message.total_slots();
  return true;
}

bool ClientCommutator::getAttachedModulesList(ModulesList& attachedModules)
{
  uint32_t nTotal = 0;
  if (!getTotalSlots(nTotal))
    return false;

  spex::Message request;
  request.mutable_commutator()->set_all_modules_info_req(true);
  if (!send(request))
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

TunnelPtr ClientCommutator::openTunnel(uint32_t nSlotId)
{
  if (!sendOpenTunnel(nSlotId))
    return TunnelPtr();

  uint32_t nOpenedTunnelId = 0;
  if (!waitOpenTunnelSuccess(&nOpenedTunnelId))
    return TunnelPtr();

  TunnelPtr pTunnel = std::make_shared<Tunnel>(nOpenedTunnelId);
  getChannel()->attachTunnelHandler(nOpenedTunnelId, pTunnel);
  pTunnel->setProceeder(getChannel()->getProceeder());
  pTunnel->attachToDownlevel(getChannel());
  return pTunnel;
}

bool ClientCommutator::sendOpenTunnel(uint32_t nSlotId)
{
  spex::Message request;
  request.mutable_commutator()->set_open_tunnel(nSlotId);
  return send(request);
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

}}  // namespace autotests::client

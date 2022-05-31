#pragma once

#include <stdint.h>
#include <vector>

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <Autotests/ClientSDK/Router.h>

namespace autotests { namespace client {

struct ModuleInfo
{
  uint32_t    nSlotId;
  std::string sModuleType;
  std::string sModuleName;
};

using ModulesList = std::vector<ModuleInfo>;

class ClientCommutator : public ClientBaseModule
{
private:
  RouterPtr m_pRouter;

public:
  ClientCommutator(RouterPtr pRouter) : m_pRouter(pRouter) {}

  bool getTotalSlots(uint32_t &nTotalSlots);
  bool getAttachedModulesList(ModulesList& attachedModules);

  Router::SessionPtr openSession(uint32_t nSlotId);
  bool closeTunnel(Router::SessionPtr pSession);

  // Additional functions, that are used in autotests:
  bool sendOpenTunnel(uint32_t nSlotId);
  bool waitOpenTunnelSuccess(uint32_t *pOpenedTunnelId = nullptr);
  bool waitOpenTunnelFailed();

  bool sendCloseTunnel(uint32_t nTunnelId);
  bool waitCloseTunnelStatus(spex::ICommutator::Status& status);
  bool waitCloseTunnelInd();

  bool sendTotalSlotsReq();
  bool waitTotalSlots(uint32_t& nSlots);

  bool waitGameOverReport(spex::IGame::GameOver &report, uint16_t nTimeout = 500);
};

using ClientCommutatorPtr = std::shared_ptr<ClientCommutator>;

}}  // namespace autotests::client

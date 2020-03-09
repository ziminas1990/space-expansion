#pragma once

#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <stdint.h>
#include <vector>

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
public:
  bool getTotalSlots(uint32_t &nTotalSlots);
  bool getAttachedModulesList(ModulesList& attachedModules);

  TunnelPtr openTunnel(uint32_t nSlotId);

  // Additional functions, that are used in autotests:
  bool sendOpenTunnel(uint32_t nSlotId);
  bool waitOpenTunnelSuccess(uint32_t *pOpenedTunnelId = nullptr);
  bool waitOpenTunnelFailed();

  bool waitGameOverReport(spex::IGame::GameOver &report, uint16_t nTimeout = 500);
};

using ClientCommutatorPtr = std::shared_ptr<ClientCommutator>;

}}  // namespace autotests::client

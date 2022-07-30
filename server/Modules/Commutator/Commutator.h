#pragma once

#include <set>
#include <vector>
#include <memory>
#include <stack>
#include <stdint.h>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Network/Fwd.h>

namespace modules {

class Commutator :
    public BaseModule,
    public utils::GlobalObject<Commutator>
{
  static const size_t m_nSessionsLimit = 8;

public:
  static uint32_t invalidSlot() { return UINT32_MAX; }

  Commutator(network::SessionMuxWeakPtr pSessionMux);

  uint32_t attachModule(BaseModulePtr pModule);
    // Attach module to commutator. Return slotId - number of slot, to which
    // the module has been attached

  bool detachModule(uint32_t nSloteId, const BaseModulePtr& pModule);
    // Detach the specified 'pModule', attached to the specified 'nSlotId'.

  BaseModulePtr findModuleByName(std::string const& sName) const;
    // Return module with the specified 'sName'. If module doesn't exist, 
    // return null. Note that call has O(n) complicity

  BaseModulePtr findModuleByType(std::string const& sType) const;
    // Return module with the specified 'sType'. If module doesn't exist, 
    // return null. Note that call has O(n) complicity

  void checkSlots();
    // Check if modules in some slots are offline now, or marked destroyed.
    // For such modules close all active connections. Detach destroyed
    // modules from commutator.

  size_t totalSlots() const { return m_modules.size(); }

  BaseModuleConstPtr moduleInSlot(size_t nSlotId) const {
    return nSlotId < m_modules.size() ? m_modules[nSlotId] : nullptr;
  }

  const std::vector<BaseModulePtr>& getAllModules() const {
    return m_modules;
  }

  void onSessionClosed(uint32_t nSessionId) override;

protected:
  // overrides from BaseModule
  void handleCommutatorMessage(uint32_t, spex::ICommutator const&) override;

private:
  // Command handlers
  void onGetTotalSlotsRequest(uint32_t nSessionId) const;
  void getModuleInfo(uint32_t nSessionId, uint32_t nSlotId) const;
  void getAllModulesInfo(uint32_t nSessionId) const;
  void onOpenTunnelRequest(uint32_t nSessionId, uint32_t nSlot);
  void onCloseTunnelRequest(uint32_t nSessionId, uint32_t nTunnelId);
  void onMonitoringRequest(uint32_t nSessionId);

  void onModuleHasBeenDetached(uint32_t nSlotId);

  void sendOpenTunnelFailed(uint32_t nSessionId, spex::ICommutator::Status eReason);
  void sendCloseTunnelStatus(uint32_t nSessionId, spex::ICommutator::Status eStatus);
  void sendMonitorStatus(uint32_t nSessionId,
                         spex::ICommutator::Status eStatus) const;
  void sendModuleAttachedUpdate(uint32_t nSlotId) const;
  void sendModuleDetachedUpdate(uint32_t nSlotId) const;

private:
  network::SessionMuxWeakPtr m_pSessionMux;
  // index - slotId
  std::vector<BaseModulePtr> m_modules;

  std::vector<uint32_t> m_monitoringSessions;
};

} // namespace modules

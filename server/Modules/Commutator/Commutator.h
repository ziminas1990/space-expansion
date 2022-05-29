#pragma once

#include <set>
#include <vector>
#include <memory>
#include <stack>
#include <stdint.h>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Network/SessionMux.h>

namespace modules {

class Commutator :
    public BaseModule,
    public utils::GlobalObject<Commutator>
{
  static const size_t m_nSessionsLimit = 8;

public:
  static uint32_t invalidSlot() { return UINT32_MAX; }

  Commutator(std::shared_ptr<network::SessionMux> pSessionMux);

  uint32_t attachModule(BaseModulePtr pModule);
    // Attach module to commutator. Return slotId - number of slot, to which
    // the module has been attached

  BaseModulePtr findModuleByName(std::string const& sName) const;
    // Return module with the specified 'sName'. If module doesn't exist, return null.
    // Note that call has O(n) complicity

  BaseModulePtr findModuleByType(std::string const& sType) const;
    // Return module with the specified 'sType'. If module doesn't exist, return null.
    // Note that call has O(n) complicity

  void detachFromModules();

  // Check if all slotes are still active; if some slot is NOT active anymore,
  // commutator will send an indication
  void checkSlots();

  // overrides from network::IPrutubufTerminal
  bool openSession(uint32_t) override { return true; }

  size_t totalSlots() const { return m_slots.size(); }

  BaseModuleConstPtr moduleInSlot(size_t nSlotId) const {
    return nSlotId < m_slots.size() ? m_slots[nSlotId].m_pModule : nullptr;
  }

protected:
  // overides from BufferedProtobufTerminal interface
  void handleMessage(uint32_t nSessionId, spex::Message const& message) override;

private:
  // Command handlers
  void onGetTotalSlotsRequest(uint32_t nSessionId) const;
  void getModuleInfo(uint32_t nSessionId, uint32_t nSlotId) const;
  void getAllModulesInfo(uint32_t nSessionId) const;
  void onOpenTunnelRequest(uint32_t nSessionId, uint32_t nSlot);
  void onCloseTunnelRequest(uint32_t nSessionId, uint32_t nTunnelId);

  void onModuleHasBeenDetached(uint32_t nSlotId);

  void sendOpenTunnelFailed(uint32_t nSessionId, spex::ICommutator::Status eReason);
  void sendCloseTunnelStatus(uint32_t nSessionId, spex::ICommutator::Status eStatus);
  void sendCloseTunnelInd(uint32_t nTunnelId);

private:
  struct Slot {
    BaseModulePtr         m_pModule;
    std::vector<uint32_t> m_activeSessions;

    bool isValid() const { return m_pModule != nullptr; }
    bool removeSessionId(uint32_t nSessionId);
    void reset();
  };

private:
  std::vector<Slot> m_slots; // index - SlotId
  std::shared_ptr<network::SessionMux> m_pSessionMux;
};

using CommutatorPtr     = std::shared_ptr<Commutator>;
using CommutatorWeakPtr = std::weak_ptr<Commutator>;

} // namespace modules

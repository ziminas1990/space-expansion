#pragma once

#include <set>
#include <vector>
#include <memory>
#include <stack>
#include <stdint.h>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>

namespace modules {

class Commutator :
    public BaseModule,
    public utils::GlobalContainer<Commutator>,
    public network::IPlayerChannel
{
  static const size_t m_nSessionsLimit = 8;

public:
  Commutator();

  uint32_t attachModule(BaseModulePtr pModule);
    // Attach module to commutator. Return slotId - number of slot, to which the module
    // has been attached

  BaseModulePtr findModuleByName(std::string const& sName) const;
    // Return module with the specified 'sName'. If module doesn't exist, return null.
    // Note that call has O(n) complicity

  BaseModulePtr findModuleByType(std::string const& sType) const;
    // Return module with the specified 'sType'. If module doesn't exist, return null.
    // Note that call has O(n) complicity

  std::vector<BaseModulePtr> getAllModules() const { return m_Slots; }

  void detachFromModules();

  // Check if all slotes are still active; if some slot is NOT active anymore,
  // commutator will send an indication
  void checkSlotsAndTunnels();

  // Send the specified 'message' to all channels, that are opened to *this* commutator.
  // Channels, that were opened to commutator's modules will NOT be used to send message.
  void broadcast(spex::Message const& message);

  // overrides from network::IPrutubufTerminal
  bool openSession(uint32_t nSessionId) override;
  void onSessionClosed(uint32_t nSessionId) override;

  // overrides from network::IProtobufChannel
  bool send(uint32_t nSessionId, spex::Message const& message) const override;
  void closeSession(uint32_t nSessionId) override;
  bool isValid() const override { return channelIsValid(); }
  void attachToTerminal(network::IPlayerTerminalPtr) override {}
  void detachFromTerminal() override;

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

  void commutateMessage(uint32_t nTunnelId, spex::Message const& message);

  void onModuleHasBeenDetached(uint32_t nSlotId);
  void onModuleHasBeenAttached(uint32_t nSlotId);

  void sendOpenTunnelFailed(uint32_t nSessionId, spex::ICommutator::Status eReason);
  void sendCloseTunnelFailed(uint32_t nSessionId, spex::ICommutator::Status eReason);

private:
  struct Tunnel {
    bool     m_lUp     = false;
    uint32_t m_nFather = 0; // Session, that created a tunnel
    uint32_t m_nSlotId = 0;
  };

private:
  // index - Slot Id, value - connected module
  std::vector<BaseModulePtr> m_Slots;
  // index - Tunnel Id, value - { Session Id, Slot Id }
  std::vector<Tunnel>   m_Tunnels;
  std::stack<uint32_t>  m_ReusableTunnels;
  // all sessions
  std::set<uint32_t>    m_OpenedSessions;

};

using CommutatorPtr     = std::shared_ptr<Commutator>;
using CommutatorWeakPtr = std::weak_ptr<Commutator>;

} // namespace modules

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
    public network::IProtobufChannel
{
  static const size_t m_nSessionsLimit = 8;

public:
  Commutator();

  uint32_t attachModule(BaseModulePtr pModule);
    // Attach module to commutator. Return slotId - number of slot, to which the module
    // has been attached

  void detachFromModules();

  // Check if all slotes are still active; if some slot is NOT active anymore,
  // commutator will send an indication
  void checkSlotsAndTunnels();

  // overrides from network::IPrutubufTerminal
  bool openSession(uint32_t nSessionId) override;
  void onSessionClosed(uint32_t nSessionId) override;

  // overrides from network::IProtobufChannel
  bool send(uint32_t nSessionId, spex::Message const& message) const override;
  void closeSession(uint32_t nSessionId) override;
  bool isValid() const override { return channelIsValid(); }
  void attachToTerminal(network::IProtobufTerminalPtr) override {}
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
  void onCloseTunnelRequest(uint32_t nTunnelId, uint32_t nSessionId = uint32_t(-1));

  void commutateMessage(uint32_t nTunnelId, spex::Message const& message);

  void onModuleHasBeenDetached(uint32_t nSlotId);
  void onModuleHasBeenAttached(uint32_t nSlotId);

private:
  struct Tunnel {
    bool     m_lUp        = false;
    uint32_t m_nSessionId = 0; // Session, that created a tunnel
    uint32_t m_nSlotId    = 0;
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

#pragma once

#include <queue>
#include <map>
#include <memory>
#include <functional>
#include "Interfaces.h"

namespace autotests { namespace client {

class SyncPipe :
    public IClientChannel,
    public IClientTerminal
{
public:

  void attachToDownlevel(IClientChannelPtr pDownlevel);
  void detachDownlevel() { m_pDownlevel.reset(); }
  void attachTunnelHandler(uint32_t nTunnelId, IClientTerminalWeakPtr pHandler);

  void setProceeder(std::function<void()> fEnviromentProceeder);
  std::function<void()> getProceeder() const { return m_fEnviromentProceeder; }

  // Waiting message in already opened session nSessionId
  bool waitAny(uint16_t nTimeoutMs = 100);
  bool waitAny(spex::Message &out, uint16_t nTimeoutMs = 100);

  bool wait(spex::IAccessPanel &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::ICommutator &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IShip &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::INavigation &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IEngine &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::ICelestialScanner &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IAsteroidScanner &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IResourceContainer &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IAsteroidMiner &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IBlueprintsLibrary& out, uint16_t nTimeoutMs = 100);

  // Expect, that no message will be received in session
  bool expectSilence(uint16_t nTimeoutMs);

  // overrides from IClientTerminal interface
  void onMessageReceived(spex::Message&& message) override;

  // overrides from IClientChannel interface
  bool send(spex::Message const& message) override;

private:
  bool waitConcrete(spex::Message::ChoiceCase eExpectedChoice,
                    spex::Message &out, uint16_t nTimeoutMs = 500);

private:
  IClientChannelPtr m_pDownlevel;
  std::map<uint32_t, IClientTerminalWeakPtr> m_handlers;
  std::function<void()> m_fEnviromentProceeder;

  std::queue<spex::Message> m_receivedMessages;
};

using SyncPipePtr = std::shared_ptr<SyncPipe>;


class Tunnel : public SyncPipe
{
public:
  Tunnel(uint32_t nTunnelId) : m_nTunnelId(nTunnelId) {}

  uint32_t getTunnelId() const { return m_nTunnelId; }

  // overrides from SyncPipe
  bool send(spex::Message const& message) override;

private:
  uint32_t m_nTunnelId;
};

using TunnelPtr = std::shared_ptr<Tunnel>;

}} // namespace autotest::client

#pragma once

#include <queue>
#include <map>
#include <memory>
#include <functional>

#include "Interfaces.h"
#include <Protocol.pb.h>
#include <Privileged.pb.h>
#include <Utils/WaitingFor.h>

namespace autotests { namespace client {

// Awesome class! For more details see the the 'waitAny()' docstring
template<typename FrameType>
class SyncPipe : public IChannel<FrameType>,
                 public ITerminal<FrameType>
{
public:
  void setProceeder(std::function<void()> fEnviromentProceeder) {
    m_fEnviromentProceeder = std::move(fEnviromentProceeder);
  }

  std::function<void()> getProceeder() const { return m_fEnviromentProceeder; }

  // waitAny() call is a key feature of SyncPipe. It blocks control until some
  // message received or the specified 'nTimeoutMs' runs out. If message is
  // received, store it to the specified 'out' and return true. Otherwise
  // return false.
  bool waitAny(FrameType &out, uint16_t nTimeoutMs)
  {
    if (!waitAny(nTimeoutMs))
      return false;
    out = std::move(m_receivedMessages.front());
    m_receivedMessages.pop();
    return true;
  }

  bool waitAny(uint16_t nTimeoutMs)
  {
    return utils::waitFor(
          [this]() { return !m_receivedMessages.empty(); },
          m_fEnviromentProceeder, nTimeoutMs);
  }

  // override from ITerminal<FrameType>
  void onMessageReceived(FrameType &&message) override
  {
    m_receivedMessages.push(std::move(message));
  }

  void attachToDownlevel(IChannelPtr<FrameType> pDownlevel) override {
    m_pDownlevel = pDownlevel;
  }

  void detachDownlevel() override { m_pDownlevel.reset(); }

  // overrides from IChannel<FrameType> interface
  bool send(FrameType const& message) override
  {
    return m_pDownlevel && m_pDownlevel->send(message);
  }

  void dropAll()
  {
    while (!m_receivedMessages.empty()) {
      m_receivedMessages.pop();
    }
  }

private:
  IChannelPtr<FrameType> m_pDownlevel;
    // Channel, that will be used to send messages
  std::function<void()>  m_fEnviromentProceeder;
    // Environment will be proceeded while pipe is waiting for message
  std::queue<FrameType>  m_receivedMessages;
    // All received messages are stored to this queue
};


class PlayerPipe : public SyncPipe<spex::Message>
{
public:

  void attachTunnelHandler(uint32_t nTunnelId, IPlayerTerminalWeakPtr pHandler);

  bool wait(spex::IAccessPanel &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::ICommutator &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IShip &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::INavigation &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IEngine &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IPassiveScanner &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::ICelestialScanner &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IAsteroidScanner &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IResourceContainer &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IAsteroidMiner &out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IBlueprintsLibrary& out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IShipyard& out, uint16_t nTimeoutMs = 100);
  bool wait(spex::IGame& out, uint16_t nTimeoutMs = 100);

  // overrides from SyncPipe<spex::Message>
  void onMessageReceived(spex::Message&& message) override;

private:
  bool waitConcrete(spex::Message::ChoiceCase eExpectedChoice,
                    spex::Message &out, uint16_t nTimeoutMs = 500);

private:
  std::map<uint32_t, IPlayerTerminalWeakPtr> m_handlers;
};

using PlayerPipePtr = std::shared_ptr<PlayerPipe>;


class Tunnel : public PlayerPipe
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


class PrivilegedPipe : public SyncPipe<admin::Message>
{
public:

  bool wait(admin::Access &out, uint16_t nTimeoutMs = 100);
  bool wait(admin::SystemClock &out, uint16_t nTimeoutMs = 100);

private:
  bool waitConcrete(admin::Message::ChoiceCase eExpectedChoice,
                    admin::Message &out, uint16_t nTimeoutMs = 500);

};

using PrivilegedPipePtr = std::shared_ptr<PrivilegedPipe>;

}} // namespace autotest::client

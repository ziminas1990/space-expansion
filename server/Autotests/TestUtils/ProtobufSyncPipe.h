#pragma once

#include <queue>
#include <map>
#include <memory>
#include <functional>
#include <Protocol.pb.h>
#include <Network/Interfaces.h>

namespace autotests {

class ProtobufSyncPipe;
using ProtobufSyncPipePtr = std::shared_ptr<ProtobufSyncPipe>;

class ProtobufSyncPipe : public network::IPlayerTerminal
{
  using MessagesQueue = std::queue<spex::Message>;
  using SessionsQueue = std::map<uint32_t, MessagesQueue>;

public:
  virtual ~ProtobufSyncPipe() override = default;

  void setEnviromentProceeder(std::function<void()> fProceeder)
  { m_fEnviromentProceeder = std::move(fProceeder); }

  // Waiting message in already opened session nSessionId
  bool waitAny(uint32_t nSessionId, uint16_t nTimeoutMs = 100);
  bool waitAny(uint32_t nSessionId, spex::Message &out, uint16_t nTimeoutMs = 100);

  bool wait(uint32_t nSessionId, spex::IAccessPanel &out, uint16_t nTimeoutMs = 100);
  bool wait(uint32_t nSessionId, spex::ICommutator &out, uint16_t nTimeoutMs = 100);
  bool wait(uint32_t nSessionId, spex::INavigation &out, uint16_t nTimeoutMs = 100);

  // Expect, that no message will be received in session
  bool expectSilence(uint32_t nSessionId, uint16_t nTimeoutMs);

  // overrides from ITerminal interface
  bool openSession(uint32_t /*nSessionId*/) override { return true; }
  void onMessageReceived(uint32_t nSessionId, spex::Message const& frame) override;
  void onSessionClosed(uint32_t /*nSessionId*/) override {}
  void attachToChannel(network::IPlayerChannelPtr pChannel) override
  { m_pAttachedChannel = pChannel; }
  void detachFromChannel() override { m_pAttachedChannel.reset(); }

private:
  bool waitConcrete(uint32_t nSessionId, spex::Message::ChoiceCase eExpectedChoice,
                    spex::Message &out, uint16_t nTimeoutMs = 500);

private:
  network::IPlayerChannelPtr  m_pAttachedChannel;
  std::function<void()>         m_fEnviromentProceeder;
  mutable SessionsQueue         m_Sessions;
};

} // namespace autotest

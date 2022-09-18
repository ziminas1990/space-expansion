#pragma once

#include <queue>
#include <map>
#include <memory>
#include <functional>
#include <type_traits>

#include "Interfaces.h"
#include <Protocol.pb.h>
#include <Privileged.pb.h>
#include <Utils/WaitingFor.h>
#include <Autotests/TestUtils/Unpackers.h>

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

  // Store to the specified 'out' message from receive buffer (if any) and
  // return true. Return false if buffer is empty.
  bool pickAny(FrameType& out)
  {
    if (m_receivedMessages.empty()) {
      return false;
    }
    out = std::move(m_receivedMessages.front());
    m_receivedMessages.pop();
    return true;
  }

  // waitAny() call is a key feature of SyncPipe. It blocks control until some
  // message received or the specified 'nTimeoutMs' runs out. If message is
  // received, store it to the specified 'out' and return true. Otherwise
  // return false.
  bool waitAny(uint16_t nTimeoutMs)
  {
    return utils::waitFor(
          [this]() { return !m_receivedMessages.empty(); },
          m_fEnviromentProceeder,
          nTimeoutMs);
  }

  bool waitAny(FrameType &out, uint16_t nTimeoutMs)
  {
    return waitAny(nTimeoutMs) && pickAny(out);
  }

  // override from ITerminal<FrameType>
  void onMessageReceived(FrameType&& message) override
  {
    if constexpr (std::is_same_v<FrameType, spex::Message>) {
      // In case heartbeat is received, it should be sent back
      if (message.choice_case() == spex::Message::kSession) {
        if (message.session().choice_case() ==
            spex::ISessionControl::kHeartbeat) {
          send(std::move(message));
          return;
        }
      }
    }
    m_receivedMessages.push(std::move(message));
  }

  void attachToDownlevel(IChannelPtr<FrameType> pDownlevel) override {
    m_pDownlevel = pDownlevel;
  }

  void detachDownlevel() override { m_pDownlevel.reset(); }

  // overrides from IChannel<FrameType> interface
  bool send(FrameType&& message) override
  {
    return m_pDownlevel && m_pDownlevel->send(std::move(message));
  }

  void attachToTerminal(ITerminalPtr<FrameType>) override
  {
    assert(nullptr == "Operation makes no sense");
  }

  void detachFromTerminal() override
  {
    assert(nullptr == "Operation makes no sense");
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

  template<typename MessageType>
  bool wait(MessageType &out, uint16_t nTimeoutMs = 100) {
    using Payload = Unpacker<MessageType>;
    spex::Message message;
    if(!waitConcrete(Payload::choice(), message, nTimeoutMs))
      return false;
    out = Payload::unpack(message);
    return true;
  }

  template<typename MessageType>
  bool pick(MessageType &out) {
    using Payload = Unpacker<MessageType>;
    spex::Message message;
    if(!pickConcrete(Payload::choice(), message))
      return false;
    out = Payload::unpack(message);
    return true;
  }

private:
  bool waitConcrete(spex::Message::ChoiceCase eExpectedChoice,
                    spex::Message &out,
                    uint16_t nTimeoutMs = 500);

  bool pickConcrete(spex::Message::ChoiceCase eExpectedChoice,
                    spex::Message &out);
};

using PlayerPipePtr = std::shared_ptr<PlayerPipe>;


class PrivilegedPipe : public SyncPipe<admin::Message>
{
public:

  // This may be replaced with template implementation, that uses
  // Unpacker<> struct (see PlayerPipe implementation).
  bool wait(admin::Access &out, uint16_t nTimeoutMs = 100);
  bool wait(admin::SystemClock &out, uint16_t nTimeoutMs = 100);

private:
  bool waitConcrete(admin::Message::ChoiceCase eExpectedChoice,
                    admin::Message &out, uint16_t nTimeoutMs = 500);

};

using PrivilegedPipePtr = std::shared_ptr<PrivilegedPipe>;

}} // namespace autotest::client

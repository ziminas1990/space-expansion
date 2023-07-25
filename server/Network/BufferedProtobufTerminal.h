#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>

#include <Utils/Clock.h>

namespace network {

// When subclassing this class, you MUST override:
// 1. IProtobufTerminal::canOpenSession()
// 2. IProtobufTerminal::openSession(sessionId)
// 3. IProtobufTerminal::onSessionClosed(sessionId)
// 4. handleMessage(nSessionId, message)
//
// The `FrameType` parameter specifies type of protobuf message, like
// `spex::Message` or `admin::Message`
template<typename FrameType>
class BufferedProtobufTerminal : public ITerminal<FrameType>
{
  using Channel    = IChannel<FrameType>;
  using ChannelPtr = std::shared_ptr<Channel>;
public:
  BufferedProtobufTerminal() { m_messages.reserve(0x40); }

  // overrides from IProtobufTerminal interface
  void onMessageReceived(uint32_t nSessionId, FrameType const& message) override;
  void attachToChannel(ChannelPtr pChannel) override { m_pChannel = pChannel; }
  void detachFromChannel() override { m_pChannel.reset(); }

  void handleBufferedMessages();

protected:
  virtual void handleMessage(uint32_t nSessionId, FrameType const& message) = 0;
  bool channelIsValid() const { return m_pChannel && m_pChannel->isValid(); }

  bool send(uint32_t nSessionId, FrameType&& message) const {
    return m_pChannel && m_pChannel->send(nSessionId, std::move(message));
  }

  void closeSession(uint32_t nSessionId) {
    if (m_pChannel) {
      m_pChannel->closeSession(nSessionId);
    }
  }

  ChannelPtr getChannel() const { return m_pChannel; }

private:
  struct BufferedMessage
  {
    BufferedMessage(uint32_t nSessionId, FrameType const& message)
      : m_nSessionId(nSessionId), m_body(message)
    {}
    uint32_t  m_nSessionId = 0;
    FrameType m_body;
  };

  void drawnDelayedMessage();

private:
  ChannelPtr                   m_pChannel;
  std::vector<BufferedMessage> m_messages;
  // Messages, that are waiting for exact time to be handled
  std::vector<BufferedMessage> m_delayedMessages;
};


using BufferedPlayerTerminal     = BufferedProtobufTerminal<spex::Message>;
using BufferedPrivilegedTerminal = BufferedProtobufTerminal<admin::Message>;


template<typename FrameType>
void BufferedProtobufTerminal<FrameType>::onMessageReceived(
    uint32_t nSessionId, FrameType const& message)
{
  m_messages.emplace_back(nSessionId, message);
}

template<typename FrameType>
void BufferedProtobufTerminal<FrameType>::handleBufferedMessages()
{
  const uint16_t delayedQueueLimit = 1024;
  const uint64_t now               = utils::GlobalClock::now();

  for(BufferedMessage& message : m_messages) {
    if (now < message.m_body.timestamp()) {
      if (m_delayedMessages.size() == delayedQueueLimit) {
        // Drop the message :(
        return;
      }
      m_delayedMessages.emplace_back(std::move(message));
      drawnDelayedMessage();
    } else {
      // Handle immediatelly
      handleMessage(message.m_nSessionId, message.m_body);
    }
  }
  m_messages.clear();

  while(!m_delayedMessages.empty()) {
    BufferedMessage& message = m_delayedMessages.back();
    const uint64_t ts = message.m_body.timestamp();
    if (ts <= now) {
      handleMessage(message.m_nSessionId, message.m_body);
      m_delayedMessages.pop_back();
    } else {
      break;
    }
  }
}

template<typename FrameType>
void BufferedProtobufTerminal<FrameType>::drawnDelayedMessage()
{
  // Here we assume, that all elements in m_delayedMessages are already
  // sorted (by descending timestamp), except the last one, which should
  // be placed to a proper position.
  const uint64_t ts = m_delayedMessages.back().m_body.timestamp();
  for (size_t i = m_delayedMessages.size() - 1; i > 0; --i) {
    if (m_delayedMessages[i-1].m_body.timestamp() < ts) {
      std::swap(m_delayedMessages[i-1], m_delayedMessages[i]);
    } else {
      return;
    }
  }
}

} // namespace network

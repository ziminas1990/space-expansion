#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>

namespace network {

// When subclassing this class, you MUST override:
// 1. IProtobufTerminal::openSession(sessionId)
// 2. IProtobufTerminal::onSessionClosed(sessionId)
// 3. handleMessage(nSessionId, message)
//
// The `FrameType` parameter specifies type of protobuf message, like `spex::Message`
// or `admin::Message`
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
  bool send(uint32_t nSessionId, FrameType const& message) const {
    return m_pChannel && m_pChannel->send(nSessionId, message);
  }

private:
  struct BufferedMessage
  {
    BufferedMessage(uint32_t nSessionId, spex::Message const& message)
      : m_nSessionId(nSessionId), m_body(message)
    {}
    uint32_t      m_nSessionId = 0;
    spex::Message m_body;
  };

private:
  std::vector<BufferedMessage> m_messages;

  ChannelPtr m_pChannel;
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
  for(BufferedMessage& message : m_messages)
    handleMessage(message.m_nSessionId, message.m_body);
  m_messages.clear();
}


} // namespace network

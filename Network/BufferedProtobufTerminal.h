#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>

namespace network {

// When subclassing this class, you MUST override:
// 1. IProtobufTerminal::openSession(sessionId)
// 2. IProtobufTerminal::onSessionClosed(sessionId)
class BufferedProtobufTerminal : public IProtobufTerminal
{
public:
  BufferedProtobufTerminal() { m_messages.reserve(0x40); }

  // overrides from IProtobufTerminal interface
  void onMessageReceived(uint32_t nSessionId, spex::Message const& message) override;
  void attachToChannel(IProtobufChannelPtr pChannel) override { m_pChannel = pChannel; }
  void detachFromChannel() override { m_pChannel.reset(); }

  void handleBufferedMessages();

protected:
  virtual void handleMessage(uint32_t nSessionId, spex::Message const& message) = 0;
  bool channelIsValid() const { return m_pChannel && m_pChannel->isValid(); }
  bool send(uint32_t nSessionId, spex::Message const& message) const {
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

  IProtobufChannelPtr m_pChannel;
};

using BufferedProtobufTerminalPtr = std::shared_ptr<BufferedProtobufTerminal>;

} // namespace network

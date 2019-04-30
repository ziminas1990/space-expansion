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
  void onMessageReceived(uint32_t nSessionId, spex::ICommutator&& message) override;
  void attachToChannel(IProtobufChannelPtr pChannel) override { m_pChannel = pChannel; }
  void detachFromChannel() override { m_pChannel.reset(); }

  void handleBufferedMessages();

protected:
  virtual void handleMessage(uint32_t nSessionId, spex::ICommutator&& message) = 0;
  bool channelIsValid() const { return m_pChannel && m_pChannel->isValid(); }
  bool send(uint32_t nSessionId, spex::ICommutator&& message) const {
    return m_pChannel && m_pChannel->send(nSessionId, std::move(message));
  }

private:
  struct BufferedMessage
  {
    BufferedMessage(size_t nSessionId, spex::ICommutator&& body)
      : m_nSessionId(nSessionId), m_body(std::move(body))
    {}

    size_t m_nSessionId = 0;
    spex::ICommutator m_body;
  };

private:
  std::vector<BufferedMessage> m_messages;

  IProtobufChannelPtr m_pChannel;
};

using BufferedProtobufTerminalPtr = std::shared_ptr<BufferedProtobufTerminal>;

} // namespace network

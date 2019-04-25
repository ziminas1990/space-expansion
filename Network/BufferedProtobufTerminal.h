#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>

namespace network {

class BufferedProtobufTerminal : public IProtobufTerminal
{
public:
  BufferedProtobufTerminal() { m_messages.reserve(0x40); }

  // overrides from IProtobufTerminal interface
  void onMessageReceived(
      size_t nSessionId, spex::CommandCenterMessage&& message) override;
  void attachToChannel(IProtobufChannelPtr pChannel) override { m_pChannel = pChannel; }
  void detachFromChannel() override { m_pChannel.reset(); }

  void handleBufferedMessages();

protected:
  virtual void handleMessage(size_t nSessionId,
                             spex::CommandCenterMessage&& message) = 0;
  bool send(size_t nSessionId, spex::CommandCenterMessage const& message) {
    return m_pChannel && m_pChannel->sendMessage(nSessionId, message);
  }

private:
  struct BufferedMessage
  {
    BufferedMessage(size_t nSessionId, spex::CommandCenterMessage&& body)
      : m_nSessionId(nSessionId), m_body(std::move(body))
    {}

    size_t m_nSessionId = 0;
    spex::CommandCenterMessage m_body;
  };

private:
  std::vector<BufferedMessage> m_messages;

  IProtobufChannelPtr m_pChannel;
};

using BufferedProtobufTerminalPtr = std::shared_ptr<BufferedProtobufTerminal>;

} // namespace network

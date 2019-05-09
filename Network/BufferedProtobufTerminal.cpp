#include "BufferedProtobufTerminal.h"

namespace network {

void BufferedProtobufTerminal::onMessageReceived(
    uint32_t nSessionId, spex::Message const& message)
{
  m_messages.emplace_back(nSessionId, message);
}

void BufferedProtobufTerminal::handleBufferedMessages() {
  for(BufferedMessage& message : m_messages)
    handleMessage(message.m_nSessionId, message.m_body);
  m_messages.clear();
}

} // namespace network

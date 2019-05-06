#include "BufferedProtobufTerminal.h"

namespace network {

void BufferedProtobufTerminal::onMessageReceived(
    uint32_t nSessionId, spex::Message&& message)
{
  m_messages.emplace_back(nSessionId, std::move(message));
}

void BufferedProtobufTerminal::handleBufferedMessages() {
  for(BufferedMessage& message : m_messages)
    handleMessage(message.m_nSessionId, std::move(message.m_body));
  m_messages.clear();
}

} // namespace network

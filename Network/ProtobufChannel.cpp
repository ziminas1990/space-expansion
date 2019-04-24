#include "ProtobufChannel.h"

namespace network {

bool ProtobufChannel::sendMessage(spex::CommandCenterMessage const& message)
{
  std::string buffer;
  message.SerializeToString(&buffer);
  return send(reinterpret_cast<MessagePtr>(buffer.data()), buffer.size());
}

void ProtobufChannel::attachToTerminal(IProtobufTerminalPtr pTerminal)
{
  m_pTerminal = pTerminal;
}

void ProtobufChannel::detachFromTerminal()
{
  m_pTerminal.reset();
}

void ProtobufChannel::handleMessage(MessagePtr pMessage, size_t nLength)
{
  spex::CommandCenterMessage message;
  if (message.ParseFromArray(pMessage, static_cast<int>(nLength)))
    m_pTerminal->onMessageReceived(std::move(message));
}

} // namespace network

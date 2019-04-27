#include "ProtobufChannel.h"

namespace network {

bool ProtobufChannel::sendMessage(size_t nSessionId, spex::ICommutator&& message)
{
  std::string buffer;
  message.SerializeToString(&buffer);
  return send(nSessionId, reinterpret_cast<MessagePtr>(buffer.data()), buffer.size());
}

void ProtobufChannel::attachToTerminal(IProtobufTerminalPtr pTerminal)
{
  m_pTerminal = pTerminal;
}

void ProtobufChannel::detachFromTerminal()
{
  m_pTerminal.reset();
}

void ProtobufChannel::handleMessage(
    size_t nSessionId, MessagePtr pMessage, size_t nLength)
{
  spex::ICommutator message;
  if (message.ParseFromArray(pMessage, static_cast<int>(nLength)))
    m_pTerminal->onMessageReceived(nSessionId, std::move(message));
}

} // namespace network

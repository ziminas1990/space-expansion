#include "ProtobufChannel.h"

namespace network {

bool ProtobufChannel::openSession(uint32_t nSessionId)
{
  return m_pTerminal && m_pTerminal->openSession(nSessionId);
}

void ProtobufChannel::onSessionClosed(uint32_t nSessionId)
{
  if (m_pTerminal)
    m_pTerminal->onSessionClosed(nSessionId);
}

bool ProtobufChannel::send(uint32_t nSessionId, spex::ICommutator&& message)
{
  std::string buffer;
  message.SerializeToString(&buffer);
  return isAttachedToChannel()
      && getChannel()->send(nSessionId, BinaryMessage(buffer.data(), buffer.size()));
}

void ProtobufChannel::attachToTerminal(IProtobufTerminalPtr pTerminal)
{
  m_pTerminal = pTerminal;
}

void ProtobufChannel::detachFromTerminal()
{
  m_pTerminal.reset();
}

void ProtobufChannel::closeSession(uint32_t nSessionId)
{
  if (isAttachedToChannel())
    getChannel()->closeSession(nSessionId);
}

bool ProtobufChannel::isValid() const
{
  return isAttachedToChannel() && getChannel()->isValid();
}

void ProtobufChannel::handleMessage(uint32_t nSessionId, BinaryMessage const& message)
{
  spex::ICommutator protobufMessage;
  if (protobufMessage.ParseFromArray(message.m_pBody,
                                     static_cast<int>(message.m_nLength)))
    m_pTerminal->onMessageReceived(nSessionId, std::move(protobufMessage));
}

} // namespace network

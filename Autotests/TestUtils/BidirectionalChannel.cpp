#include "BidirectionalChannel.h"
#include <iostream>

namespace autotests
{

void BidirectionalChannel::attachToClientSide(client::IClientTerminalWeakPtr pClientLink)
{
  m_pClientLink = pClientLink;
}

bool BidirectionalChannel::send(uint32_t, spex::Message const& message) const
{
  //std::cout << "Server -> Client:\n" << message.DebugString() << std::endl;
  client::IClientTerminalPtr pTerminal = m_pClientLink.lock();
  if (!pTerminal)
    return false;
  pTerminal->onMessageReceived(spex::Message(message));
  return true;
}

bool BidirectionalChannel::isValid() const
{
  return m_pClientLink.lock() != client::IClientTerminalPtr();
}

void BidirectionalChannel::attachToTerminal(network::IProtobufTerminalPtr pServer)
{
  m_pServer = pServer;
}

bool BidirectionalChannel::send(const spex::Message &message)
{
  //std::cout << "Client -> Server:\n" << message.DebugString() << std::endl;
  if (!m_pServer)
    return false;
  m_pServer->onMessageReceived(0, message);
  return true;
}

} // namespace autotests

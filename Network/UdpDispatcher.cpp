#include "UdpDispatcher.h"

namespace network {

UdpDispatcher::UdpDispatcher(boost::asio::io_service& ioContext,
                             uint16_t nPoolBegin, uint16_t nPoolEnd)
  : m_IOContext(ioContext),
    m_portsPool(nPoolBegin, nPoolEnd)
{}

UdpSocketPtr UdpDispatcher::createUdpConnection(
    BufferedTerminalPtr pTerminal, uint16_t nLocalPort)
{
  std::lock_guard<utils::Mutex> guard(m_Mutex);

  if (!nLocalPort) {
    nLocalPort = m_portsPool.getNext();
    if (!nLocalPort)
      return UdpSocketPtr();
  }

  UdpSocketPtr pUdpSocket = std::make_shared<UdpSocket>(m_IOContext, nLocalPort);
  pUdpSocket->attachToTerminal(pTerminal);
  pTerminal->attachToChannel(pUdpSocket);
  m_Connections.emplace_back(pUdpSocket, std::move(pTerminal));
  return pUdpSocket;
}

bool UdpDispatcher::prephareStage(uint16_t)
{
  // Move to proceedStage
  if (!m_IOContext.poll())
      return false;
  while(m_IOContext.poll());

  m_nNextConnectionId.store(0);
  return true;
}

void UdpDispatcher::proceedStage(uint16_t, uint32_t)
{
  size_t nConnectionId = m_nNextConnectionId.fetch_add(1);
  while (nConnectionId < m_Connections.size()) {
    m_Connections[nConnectionId].m_pTerminal->handleBufferedMessages();
    nConnectionId = m_nNextConnectionId.fetch_add(1);
  }
}

} // namespace network

#include "ConnectionManager.h"
#include "UdpSocket.h"

namespace network {

ConnectionManager::ConnectionManager(boost::asio::io_service& ioContext)
  : m_IOContext(ioContext),
    m_portsPool(36000, 36000)
{}

void ConnectionManager::registerConnection(
    IChannelPtr pChannel, BufferedTerminalPtr pTerminal)
{
  std::lock_guard<std::mutex> guard(m_Mutex);
  addConnection(pChannel, pTerminal);
}

UdpEndPoint ConnectionManager::createUdpConnection(
    UdpEndPoint &&remote, BufferedTerminalPtr pTerminal)
{
  std::lock_guard<std::mutex> guard(m_Mutex);

  uint16_t nLocalPort = m_portsPool.getNext();
  if (!nLocalPort)
    return UdpEndPoint();

  UdpSocketPtr pUdpSocket = std::make_shared<UdpSocket>(m_IOContext, nLocalPort, remote);
  addConnection(pUdpSocket, pTerminal);
  return pUdpSocket->getNativeSocket().local_endpoint();
}

bool ConnectionManager::prephareStage(uint16_t)
{
  if (!m_IOContext.poll())
      return false;
  while(m_IOContext.poll());
  m_nNextConnectionId.store(0);
  return true;
}

void ConnectionManager::proceedStage(uint16_t, size_t)
{
  size_t nConnectionId = m_nNextConnectionId.fetch_add(1);
  while (nConnectionId < m_Connections.size()) {
    m_Connections[nConnectionId].m_pTerminal->handleBufferedMessages();
    nConnectionId = m_nNextConnectionId.fetch_add(1);
  }
}

void ConnectionManager::addConnection(IChannelPtr pChannel, BufferedTerminalPtr pTerminal)
{
  pChannel->attachToTerminal(pTerminal);
  pTerminal->attachToChannel(pChannel);
  m_Connections.emplace_back(std::move(pChannel), std::move(pTerminal));
}

} // namespace network

#include "UdpServer.h"

namespace network
{

void UdpServer::addHandlerOnPort(uint16_t nPort)
{
  UdpSocketUptr pSocket = std::make_unique<UdpSocket>(m_IOContext, nPort);
  m_Sockets.push_back(std::move(pSocket));
}

bool UdpServer::prephareStage(uint16_t)
{
  if (!m_IOContext.poll())
    return false;
  while(m_IOContext.poll());
  m_nNextSocketId.store(0);
  return true;
}

void UdpServer::proceedStage(uint16_t, size_t)
{
  size_t nSocketId = m_nNextSocketId.fetch_add(1);
  UdpSocketUptr& pSocket = m_Sockets[nSocketId];
  pSocket->handleReceivedMessages(
        [&pSocket](MessagePtr pMessage, size_t nLength)
        {
          pSocket->sendMessage(pMessage, nLength);
        });
}

} // namespace network

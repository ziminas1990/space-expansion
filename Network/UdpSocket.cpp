#include "UdpSocket.h"

#include <boost/array.hpp>

namespace network {

UdpSocket::UdpSocket(boost::asio::io_service &io_context, uint16_t nLocalPort)
  : m_socket(io_context, udp::endpoint(udp::v4(), nLocalPort)),
    m_nReceiveBufferSize(8196),
    m_pReceiveBuffer(new uint8_t[m_nReceiveBufferSize])
{
  boost::asio::socket_base::reuse_address optReuseAddr(true);
  m_socket.set_option(optReuseAddr);
  receivingData();
}

void UdpSocket::addRemote(udp::endpoint const& remote)
{
  m_WhiteList.insert(remote);
  if (m_WhiteList.size() == 1) {
    // Closing all existing sessions expect one with specified remote
    for(size_t i = 0; i < m_nSessionsLimit; ++i) {
      if (m_Sessions[i] != remote)
        m_Sessions[i] = udp::endpoint();
    }
  }
}

void UdpSocket::removeRemote(udp::endpoint const& remote)
{
  m_WhiteList.erase(remote);
  // If there is a session for remote, we should close them
  for(size_t i = 0; i < m_nSessionsLimit; ++i) {
    if (m_Sessions[i] == remote)
      m_Sessions[i] = udp::endpoint();
  }
}

void UdpSocket::attachToTerminal(IBinaryTerminalPtr pTerminal)
{
  m_pTerminal = pTerminal;
}

bool UdpSocket::send(uint32_t nSessionId, BinaryMessage&& message) const
{
  std::lock_guard<std::mutex> guard(m_Mutex);

  if (nSessionId >= m_Sessions.size())
    return false;
  udp::endpoint const& remote = m_Sessions[nSessionId];
  if (remote == udp::endpoint())
    return false;

  uint8_t* pChunk = m_ChunksPool.get(message.m_nLength);
  if (!pChunk)
    pChunk = new uint8_t[message.m_nLength];
  memcpy(pChunk, message.m_pBody, message.m_nLength);
  m_socket.async_send_to(
        boost::asio::buffer(message.m_pBody, message.m_nLength), remote,
        [this, pChunk](const boost::system::error_code&, std::size_t) {
          if (!m_ChunksPool.release(pChunk))
            delete [] pChunk;
        });
  return true;
}

void UdpSocket::closeSession(uint32_t nSessionId)
{
  if (nSessionId < m_nSessionsLimit) {
    m_WhiteList.erase(m_Sessions[nSessionId]);
    m_Sessions[nSessionId] = udp::endpoint();
  }
}

void UdpSocket::receivingData()
{
  using namespace std::placeholders;
  m_socket.async_receive_from(
        boost::asio::buffer(m_pReceiveBuffer, m_nReceiveBufferSize),
        m_senderAddress,
        std::bind(&UdpSocket::onDataReceived, this, _1, _2));
}

void UdpSocket::onDataReceived(boost::system::error_code const& error,
                               std::size_t nTotalBytes)
{
  // Linear complicity in searching for sessionId is OK, because in general we won't
  // have a lot of sessions (nSessionsLimit is just 8)
  if (!error)
  { 
    // Continue receiving data
    receivingData();

    for(uint32_t nSessionId = 0; nSessionId <= m_nSessionsLimit; ++nSessionId) {
      if (m_senderAddress == m_Sessions[nSessionId]) {
        m_pTerminal->onMessageReceived(
              nSessionId, BinaryMessage(m_pReceiveBuffer, nTotalBytes));
        return;
      }
    }

    // It seems, that it is the first message from m_senderAddress
    if (!m_WhiteList.empty() && m_WhiteList.find(m_senderAddress) == m_WhiteList.end()) {
      // This remote address is NOT allowed
      return;
    }

    // Looking for free sessionId
    for(uint32_t nSessionId = 0; nSessionId <= m_nSessionsLimit; ++nSessionId) {
      if (m_Sessions[nSessionId] == udp::endpoint() &&
          m_pTerminal->openSession(nSessionId))
      {
        m_Sessions[nSessionId] = m_senderAddress;
        m_pTerminal->onMessageReceived(
              nSessionId, BinaryMessage(m_pReceiveBuffer, nTotalBytes));
        break;
      }
    }
  }
}

} // namespace network

#include "UdpSocket.h"

namespace network {

UdpSocket::UdpSocket(boost::asio::io_service &io_context, uint16_t nLocalPort,
                     boost::asio::ip::udp::endpoint const& remoteAddress)
  : m_socket(io_context, udp::endpoint(udp::v4(), nLocalPort)),
    m_remoteAddress(remoteAddress),
    m_nReceiveBufferSize(8196),
    m_pReceiveBuffer(new uint8_t[m_nReceiveBufferSize])
{
  receivingData();
}

void UdpSocket::attachToTerminal(ITerminalPtr pTerminal)
{
  m_pTerminal = pTerminal;
}

bool UdpSocket::sendMessage(MessagePtr pMessage, size_t nLength)
{
  uint8_t* pChunk = m_ChunksPool.get(nLength);
  if (pChunk) {
    memcpy(pChunk, pMessage, nLength);
    m_socket.async_send_to(
          boost::asio::buffer(pMessage, nLength), m_remoteAddress,
          [this, pChunk](const boost::system::error_code&, std::size_t) {
            m_ChunksPool.release(pChunk);
          });
  } else {
    pChunk = new uint8_t[nLength];
    memcpy(pChunk, pMessage, nLength);
    m_socket.async_send_to(
          boost::asio::buffer(pMessage, nLength), m_remoteAddress,
          [pChunk](const boost::system::error_code&, std::size_t) {
            delete [] pChunk;
          });
  }
  return true;
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
  if (!error)
  {
    if (m_remoteAddress != udp::endpoint() && m_remoteAddress != m_senderAddress)
      return;

    if (m_pTerminal)
      m_pTerminal->onMessageReceived(m_pReceiveBuffer, nTotalBytes);

    receivingData();
  }
}

} // namespace network

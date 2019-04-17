#include "UdpSocket.h"

namespace network {

UdpSocket::UdpSocket(boost::asio::io_service &io_context, uint16_t nLocalPort,
                     boost::asio::ip::udp::endpoint expectedRemoteSide)
  : m_socket(io_context, udp::endpoint(udp::v4(), nLocalPort)),
    m_expectedRemoteSide(expectedRemoteSide),
    m_nTotalMessages(0),
    m_nBytesReceived(0),
    m_nBytesLeft(m_nReceiveBufferLimit),
    m_pReceiveBuffer(new uint8_t[m_nReceiveBufferLimit])
{
  waitingForData();
}

UdpSocket::~UdpSocket()
{
  delete [] m_pReceiveBuffer;
}

void UdpSocket::handleReceivedMessages(MessageHandler&& handler)
{
  uint8_t* pMessage = m_pReceiveBuffer;
  for(size_t i = 0; i < m_nTotalMessages; ++i)
  {
    handler(pMessage, m_nMessagesLength[i]);
    pMessage += m_nMessagesLength[i];
  }
  m_nTotalMessages  = 0;
  m_nBytesLeft     += m_nBytesReceived;
  m_nBytesReceived  = 0;
  m_socket.cancel();
  waitingForData();
}

void UdpSocket::sendMessage(MessagePtr pMessage, size_t nLength)
{
  m_socket.async_send_to(
        boost::asio::buffer(pMessage, nLength),
        (m_expectedRemoteSide != udp::endpoint()) ? m_expectedRemoteSide : m_remoteSide,
        [](const boost::system::error_code&, std::size_t) {}
  );
}

void UdpSocket::waitingForData()
{
  using namespace std::placeholders;
  m_socket.async_receive_from(
        boost::asio::buffer(m_pReceiveBuffer + m_nBytesReceived, m_nBytesLeft),
        m_remoteSide,
        std::bind(&UdpSocket::onDataReceived, this, _1, _2));
}

void UdpSocket::onDataReceived(boost::system::error_code const& error,
                               std::size_t nTotalBytes)
{
  if (m_nTotalMessages == m_nReceiveMessagesLimit ||
      nTotalBytes > m_nBytesLeft)
    return;
  if (!error)
  {
    if (m_expectedRemoteSide != udp::endpoint() && m_expectedRemoteSide != m_remoteSide)
      return;

    m_nBytesReceived += nTotalBytes;
    m_nBytesLeft     -= nTotalBytes;
    m_nMessagesLength[m_nTotalMessages++] = nTotalBytes;
    waitingForData();
  }
}

} // namespace network

#include "ClientUdpSocket.h"

namespace autotests
{

ClientUdpSocket::ClientUdpSocket(boost::asio::io_service &io_context,
                                 udp::endpoint localAddress)
  : m_socket(io_context, localAddress),
    m_nReceiveBufferSize(8196),
    m_pReceiveBuffer(new uint8_t[m_nReceiveBufferSize])
{
  boost::asio::socket_base::reuse_address optReuseAddr(true);
  m_socket.set_option(optReuseAddr);
  receivingData();
}

ClientUdpSocket::~ClientUdpSocket()
{
  m_socket.close();
  delete [] m_pReceiveBuffer;
}

void ClientUdpSocket::setServerAddress(boost::asio::ip::udp::endpoint serverAddress)
{
  m_serverAddress = std::move(serverAddress);
}

void ClientUdpSocket::attachToTerminal(network::IBinaryTerminalPtr pTerminal)
{
  m_pTerminal = pTerminal;
}

bool ClientUdpSocket::send(uint32_t /*nSessionId*/,
                           network::BinaryMessage const& message) const
{
  uint8_t* pChunk = new uint8_t[message.m_nLength];
  memcpy(pChunk, message.m_pBody, message.m_nLength);
  m_socket.async_send_to(
        boost::asio::buffer(message.m_pBody, message.m_nLength), m_serverAddress,
        [pChunk](const boost::system::error_code&, std::size_t nSize) {
          delete [] pChunk;
        });
  return true;
}

void ClientUdpSocket::receivingData()
{
  using namespace std::placeholders;
  m_socket.async_receive_from(
        boost::asio::buffer(m_pReceiveBuffer, m_nReceiveBufferSize),
        m_senderAddress,
        std::bind(&ClientUdpSocket::onDataReceived, this, _1, _2));
}

void ClientUdpSocket::onDataReceived(boost::system::error_code const& error,
                                     std::size_t nTotalBytes)
{
  if (!error)
  {
    m_pTerminal->onMessageReceived(
          0, network::BinaryMessage(m_pReceiveBuffer, nTotalBytes));
    // Continue receiving data
    receivingData();
  }
}

} // namespace autotests

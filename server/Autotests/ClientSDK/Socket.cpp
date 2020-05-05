#include "Socket.h"

namespace autotests { namespace client {

Socket::Socket(boost::asio::io_service &io_context, udp::endpoint localAddress)
  : m_socket(io_context, localAddress),
    m_nReceiveBufferSize(8196),
    m_pReceiveBuffer(new uint8_t[m_nReceiveBufferSize])
{
  boost::asio::socket_base::reuse_address optReuseAddr(true);
  m_socket.set_option(optReuseAddr);
  receivingData();
}

Socket::~Socket()
{
  m_socket.close();
  delete [] m_pReceiveBuffer;
}

void Socket::setServerAddress(boost::asio::ip::udp::endpoint serverAddress)
{
  m_serverAddress = std::move(serverAddress);
}

void Socket::send(std::string const& buffer)
{
  uint8_t* pChunk = new uint8_t[buffer.size()];
  memcpy(pChunk, buffer.data(), buffer.size());
  m_socket.async_send_to(
        boost::asio::buffer(pChunk, buffer.size()), m_serverAddress,
        [pChunk](const boost::system::error_code&, std::size_t) {
          delete [] pChunk;
        });
}

void Socket::receivingData()
{
  using namespace std::placeholders;
  m_socket.async_receive_from(
        boost::asio::buffer(m_pReceiveBuffer, m_nReceiveBufferSize),
        m_senderAddress,
        std::bind(&Socket::onDataReceived, this, _1, _2));
}

void Socket::onDataReceived(boost::system::error_code const& error,
                            std::size_t nTotalBytes)
{
  if (!error)
  {
    handleData(m_pReceiveBuffer, nTotalBytes);
    // Continue receiving data
    receivingData();
  }
}

}}  // namespace autotests::client

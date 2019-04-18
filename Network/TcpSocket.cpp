#include "TcpSocket.h"

namespace network {

TcpSocket::TcpSocket(boost::asio::io_service &io_context)
  : m_socket(io_context),
    m_nReceiveBufferSize(8196),
    m_pReceiveBuffer(new uint8_t[m_nReceiveBufferSize])
{
  receivingData();
}

void TcpSocket::attachToTerminal(ITerminalPtr pTerminal)
{
  m_pTerminal = pTerminal;
}

bool TcpSocket::sendMessage(MessagePtr pMessage, size_t nLength)
{
  uint8_t* pChunk = m_ChunksPool.get(nLength);
  if (pChunk) {
    memcpy(pChunk, pMessage, nLength);
    m_socket.async_send(
          boost::asio::buffer(pMessage, nLength),
          [this, pChunk](const boost::system::error_code&, std::size_t) {
            m_ChunksPool.release(pChunk);
          });
  } else {
    pChunk = new uint8_t[nLength];
    memcpy(pChunk, pMessage, nLength);
    m_socket.async_send(
          boost::asio::buffer(pMessage, nLength),
          [pChunk](const boost::system::error_code&, std::size_t) {
            delete [] pChunk;
          });
  }
  return true;
}

void TcpSocket::receivingData()
{
  using namespace std::placeholders;
  m_socket.async_receive(
        boost::asio::buffer(m_pReceiveBuffer, m_nReceiveBufferSize),
        std::bind(&TcpSocket::onDataReceived, this, _1, _2));
}

void TcpSocket::onDataReceived(boost::system::error_code const& error,
                               std::size_t nTotalBytes)
{
  if (!error)
  {
    if (m_pTerminal)
      m_pTerminal->onMessageReceived(m_pReceiveBuffer, nTotalBytes);
    receivingData();
  }
}

} // namespace network

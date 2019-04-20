#include "TcpListener.h"
#include "TcpSocket.h"

#include <iostream>

namespace network {

TcpListener::TcpListener(boost::asio::io_service& ioContext, uint16_t nLocalTcpPort)
  : m_IOContext(ioContext),
    m_ListenSocket(ioContext, tcp::endpoint(tcp::v4(), nLocalTcpPort))
{}

void TcpListener::startListening()
{
  m_pNewConnectionSocket = std::make_shared<TcpSocket>(m_IOContext);
  m_ListenSocket.async_accept(
        m_pNewConnectionSocket->getNativeSocket(),
        std::bind(&TcpListener::onNewConnection, this, std::placeholders::_1));
}

void TcpListener::onNewConnection(boost::system::error_code const& error)
{
  if (error) {
    std::cerr << "TcpListener: " << error << std::endl;
    return;
  }

  IBufferedTerminalFactoryPtr pFactory = m_pTerminalFactory.lock();
  if (pFactory) {
    BufferedTerminalPtr pTerminal = pFactory->make();
    if (m_pConnectionManager && pTerminal) {
      m_pNewConnectionSocket->startReceiving();
      m_pConnectionManager->addConnection(m_pNewConnectionSocket, pTerminal);
    }
  }
  m_pNewConnectionSocket.reset();
  startListening();
}

} // namespace network

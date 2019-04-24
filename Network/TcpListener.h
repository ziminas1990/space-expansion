#pragma once

#include <memory>
#include <boost/asio.hpp>
#include "BufferedTerminal.h"
#include "ConnectionManager.h"
#include "TcpSocket.h"

namespace network {

// NOTE: TcpListener doesn't proceed ioContext! It should be done by someone else
class TcpListener
{
  using tcp = boost::asio::ip::tcp;
public:
  TcpListener(boost::asio::io_service& ioContext, uint16_t nLocalTcpPort);

  void attachToTerminalFactory(IBufferedTerminalFactoryWeakPtr pTerminalFactory)
  { m_pTerminalFactory = pTerminalFactory; }
  void attachToConnectionManager(ConnectionManagerPtr pConnectionManager)
  { m_pConnectionManager = pConnectionManager; }

  void start() { startListening(); }

private:
  void startListening();
  void onNewConnection(boost::system::error_code const& error);

private:
  boost::asio::io_service&        m_IOContext;
  tcp::acceptor                   m_ListenSocket;
  IBufferedTerminalFactoryWeakPtr m_pTerminalFactory;
  ConnectionManagerPtr            m_pConnectionManager;

  TcpSocketPtr m_pNewConnectionSocket;
};

using LogincLogicPtr = std::shared_ptr<TcpListener>;

} // namespace network

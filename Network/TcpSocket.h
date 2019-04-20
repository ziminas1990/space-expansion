#pragma once

#include <memory>
#include <thread>
#include "ChunksPool.h"
#include "Interfaces.h"

#include <boost/asio.hpp>

namespace network
{

class TcpSocket : public IChannel
{
  using tcp = boost::asio::ip::tcp;
public:
  TcpSocket(boost::asio::io_service& io_context);
  TcpSocket(TcpSocket const& other) = delete;
  TcpSocket(TcpSocket&& other)      = delete;
  ~TcpSocket() override { m_socket.close(); }

  void startReceiving() { receivingData(); }

  // overrides from IChannel
  bool isValid() const override { return m_socket.is_open(); }
  void attachToTerminal(ITerminalPtr pTerminal) override;
  void detachFromTerminal() override { m_pTerminal.reset(); }

  // Message pMessage will be copied to internal buffer (probably, without allocation)
  bool sendMessage(MessagePtr pMessage, size_t nLength) override;

  tcp::socket& getNativeSocket() { return m_socket; }

private:
  void receivingData();

  void onDataReceived(boost::system::error_code const& error, std::size_t nTotalBytes);
private:
  tcp::socket    m_socket;

  ITerminalPtr   m_pTerminal;

  size_t         m_nReceiveBufferSize;
  uint8_t*       m_pReceiveBuffer;
  ChunksPool     m_ChunksPool;
};

using TcpSocketPtr = std::shared_ptr<TcpSocket>;

} // namespace network

#pragma once

#include <memory>
#include <thread>
#include "ChunksPool.h"
#include "Interfaces.h"

#include <boost/asio.hpp>

namespace network
{

class UdpSocket : public IChannel
{
  using udp = boost::asio::ip::udp;
public:
  UdpSocket(boost::asio::io_service& io_context, uint16_t nLocalPort,
            udp::endpoint const& remoteAddress);
  UdpSocket(UdpSocket const& other) = delete;
  UdpSocket(UdpSocket&& other)      = delete;

  // overrides from IChannel
  bool isValid() const override { return m_socket.is_open(); }
  void attachToTerminal(ITerminalPtr pTerminal) override;
  void detachFromTerminal() override { m_pTerminal.reset(); }

  // Message pMessage will be copied to internal buffer (probably, without allocation)
  bool sendMessage(MessagePtr pMessage, size_t nLength) override;

  udp::socket& getNativeSocket() { return m_socket; }

private:
  void receivingData();

  void onDataReceived(boost::system::error_code const& error, std::size_t nTotalBytes);
private:
  udp::socket    m_socket;
  udp::endpoint  m_remoteAddress;
  udp::endpoint  m_senderAddress;

  ITerminalPtr   m_pTerminal;

  size_t         m_nReceiveBufferSize;
  uint8_t*       m_pReceiveBuffer;
  ChunksPool     m_ChunksPool;
};

using UdpSocketPtr  = std::shared_ptr<UdpSocket>;
using UdpSocketUptr = std::unique_ptr<UdpSocket>;

} // namespace network

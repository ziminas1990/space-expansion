#pragma once

#include <memory>
#include <queue>
#include <vector>
#include "Interfaces.h"

#include <boost/array.hpp>
#include <boost/asio.hpp>

namespace network
{

class UdpSocket : public IChannel
{
  using udp = boost::asio::ip::udp;

  static const size_t m_nReceiveBufferLimit   = 0xFFFF;
  static const size_t m_nReceiveMessagesLimit = 0xFF;
public:

  UdpSocket(boost::asio::io_service& io_context, uint16_t nLocalPort,
            udp::endpoint expectedRemoteSide = udp::endpoint());
  ~UdpSocket();

  void handleReceivedMessages(MessageHandler&& handler);

  // Overrides from IChannel interface
  void sendMessage(MessagePtr pMessage, size_t nLength) override;

private:
  void waitingForData();

  void onDataReceived(boost::system::error_code const& error, std::size_t nTotalBytes);

private:
  udp::socket   m_socket;
  udp::endpoint m_expectedRemoteSide;

  udp::endpoint m_remoteSide;

  size_t   m_nTotalMessages;
  size_t   m_nBytesReceived;
  size_t   m_nBytesLeft;
  uint8_t* m_pReceiveBuffer;
  size_t   m_nMessagesLength[m_nReceiveMessagesLimit];
};

using UdpSocketUptr = std::unique_ptr<UdpSocket>;

} // namespace network

#pragma once

#include "Interfaces.h"

#include <boost/asio.hpp>

namespace autotests { namespace client {

class Socket : public IClientChannel
{
  using udp = boost::asio::ip::udp;
public:
  Socket(boost::asio::io_service& io_context, udp::endpoint localAddress);
  Socket(Socket const& other) = delete;
  Socket(Socket&& other)      = delete;
  ~Socket() override;

  void setServerAddress(udp::endpoint serverAddress);
  void attachToTerminal(IClientTerminalWeakPtr pTerminal)
  { m_pTerminalLink = pTerminal; }

  // overrides from IClientChannel
  bool send(spex::Message const& message) override;

private:
  void receivingData();
  void onDataReceived(boost::system::error_code const& error, std::size_t nTotalBytes);

private:
  mutable udp::socket m_socket;
  udp::endpoint       m_senderAddress;
  udp::endpoint       m_serverAddress;

  IClientTerminalWeakPtr m_pTerminalLink;

  size_t   m_nReceiveBufferSize;
  uint8_t* m_pReceiveBuffer;
};

using SocketPtr  = std::shared_ptr<Socket>;

}}  // namespace autotests::client

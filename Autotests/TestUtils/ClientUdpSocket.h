#pragma once

#include <array>
#include <memory>
#include <thread>
#include <Utils/ChunksPool.h>
#include <Utils/Mutex.h>
#include <Network/Interfaces.h>

#include <boost/asio.hpp>

namespace autotests
{

class ClientUdpSocket : public network::IBinaryChannel
{
  using udp = boost::asio::ip::udp;
public:
  ClientUdpSocket(boost::asio::io_service& io_context, udp::endpoint localAddress);
  ClientUdpSocket(ClientUdpSocket const& other) = delete;
  ClientUdpSocket(ClientUdpSocket&& other)      = delete;
  ~ClientUdpSocket() override;

  void setServerAddress(udp::endpoint serverAddress);

  // overrides from IChannel
  bool isValid() const override { return m_socket.is_open(); }
  void attachToTerminal(network::IBinaryTerminalPtr pTerminal) override;
  void detachFromTerminal() override { m_pTerminal.reset(); }

  // Message pMessage will be copied to internal buffer (probably, without allocation)
  bool send(uint32_t nSessionId, network::BinaryMessage const& message) const override;
  void closeSession(uint32_t /*nSessionId*/) override {}

private:
  void receivingData();
  void onDataReceived(boost::system::error_code const& error, std::size_t nTotalBytes);

private:
  mutable udp::socket m_socket;
  udp::endpoint       m_senderAddress;
  udp::endpoint       m_serverAddress;

  network::IBinaryTerminalPtr m_pTerminal;

  size_t   m_nReceiveBufferSize;
  uint8_t* m_pReceiveBuffer;
};

using ClientUdpSocketPtr  = std::shared_ptr<ClientUdpSocket>;

} // namespace autotests

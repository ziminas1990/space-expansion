#pragma once

#include <array>
#include <memory>
#include <thread>
#include <Utils/ChunksPool.h>
#include "Interfaces.h"

#include <boost/asio.hpp>

namespace network
{

class UdpSocket : public IBinaryChannel
{
  using udp = boost::asio::ip::udp;
public:
  UdpSocket(boost::asio::io_service& io_context, uint16_t nLocalPort);
  UdpSocket(UdpSocket const& other) = delete;
  UdpSocket(UdpSocket&& other)      = delete;

  void addRemote(udp::endpoint const& remote);
  void removeRemote(udp::endpoint const& remote);

  // overrides from IChannel
  bool isValid() const override { return m_socket.is_open(); }
  void attachToTerminal(IBinaryTerminalPtr pTerminal) override;
  void detachFromTerminal() override { m_pTerminal.reset(); }

  // Message pMessage will be copied to internal buffer (probably, without allocation)
  bool send(uint32_t nSessionId, BinaryMessage const& message) const override;
  void closeSession(uint32_t nSessionId) override;

  udp::socket& getNativeSocket() { return m_socket; }

private:
  void receivingData();

  void onDataReceived(boost::system::error_code const& error, std::size_t nTotalBytes);
private:
  static const size_t m_nSessionsLimit = 8;

  mutable udp::socket m_socket;
  udp::endpoint       m_senderAddress;

  // set of all remote clients, whom messages would be handled
  // if empty, than messages from everyone would be handled
  std::set<udp::endpoint> m_WhiteList;

  std::array<udp::endpoint, m_nSessionsLimit> m_Sessions;

  IBinaryTerminalPtr m_pTerminal;

  size_t   m_nReceiveBufferSize;
  uint8_t* m_pReceiveBuffer;
  mutable utils::ChunksPool  m_ChunksPool;

  mutable std::mutex m_Mutex;
};

using UdpSocketPtr  = std::shared_ptr<UdpSocket>;
using UdpSocketUptr = std::unique_ptr<UdpSocket>;

} // namespace network

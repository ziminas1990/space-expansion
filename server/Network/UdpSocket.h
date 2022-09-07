#pragma once

#include <array>
#include <memory>
#include <thread>
#include <vector>
#include <optional>

#include <boost/asio.hpp>

#include <Utils/ChunksPool.h>
#include <Utils/Mutex.h>
#include "Interfaces.h"

namespace network
{

class UdpSocket : public IBinaryChannel
{
  using udp = boost::asio::ip::udp;
public:
  UdpSocket(boost::asio::io_service& io_context,
            uint16_t                 nLocalPort,
            bool                     lPromiscMode);
  UdpSocket(UdpSocket const& other) = delete;
  UdpSocket(UdpSocket&& other)      = delete;
  ~UdpSocket() override;

  udp::endpoint getLocalAddr() const {
    return m_socket.local_endpoint();
  }

  // Add the specified 'remote' and return a sessionId, associated with it.
  std::optional<uint32_t> createPersistentSession(udp::endpoint const& remote);

  // Return sessionId, associated with the specified 'remote' or nullopt,
  // if there is no such persistent session.
  std::optional<uint32_t> getSessionId(udp::endpoint const& remote) const;

  // Get remote address for the session, specified by 'nSessionId' or nullopt,
  // if such session doesn't exist.
  std::optional<udp::endpoint> getRemoteAddr(uint32_t nSessionId) const;

  // overrides from IChannel
  bool isValid() const override { return m_socket.is_open(); }
  void attachToTerminal(IBinaryTerminalPtr pTerminal) override;
  void detachFromTerminal() override { m_pTerminal.reset(); }

  // Message pMessage will be copied to internal buffer (probably, without allocation)
  bool send(uint32_t nSessionId, BinaryMessage&& message) override;
  void closeSession(uint32_t nSessionId) override;

private:
  void receivingData();

  void onDataReceived(boost::system::error_code const& error, std::size_t nTotalBytes);
private:

  mutable udp::socket m_socket;
  udp::endpoint       m_senderAddress;
  bool                m_lPromiscMode;

  std::vector<udp::endpoint> m_sessions;

  IBinaryTerminalPtr m_pTerminal;

  std::vector<uint8_t>      m_pReceiveBuffer;
  mutable utils::ChunksPool m_ChunksPool;

  mutable utils::Mutex m_Mutex;
};

} // namespace network

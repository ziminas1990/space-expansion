#include "UdpSocket.h"

#include <boost/array.hpp>

namespace network {

constexpr size_t nPersistentSessionsLimit = 8;
constexpr size_t nSessionsLimit           = 512;
constexpr size_t nReceiveBufferSize       = 8196;

UdpSocket::UdpSocket(boost::asio::io_service &io_context,
                     uint16_t                 nLocalPort,
                     bool                     lPromiscMode)
  : m_socket(io_context, udp::endpoint(udp::v4(), nLocalPort)),
    m_lPromiscMode(lPromiscMode),
    m_sessions(nSessionsLimit),
    m_pReceiveBuffer(nReceiveBufferSize)
{
  boost::asio::socket_base::reuse_address optReuseAddr(true);
  m_socket.set_option(optReuseAddr);
  receivingData();
}

UdpSocket::~UdpSocket()
{
  m_socket.close();
}

std::optional<uint32_t> 
UdpSocket::createPersistentSession(udp::endpoint const& remote)
{
  // Looking for free sessionId
  for(uint32_t i = 0; i < nPersistentSessionsLimit; ++i) {
    if (m_sessions[i] == udp::endpoint() &&
        m_pTerminal->openSession(i))
    {
      m_sessions[i] = remote;
      return i;
    }
  }
  return std::nullopt;
}

std::optional<UdpSocket::udp::endpoint>
UdpSocket::getRemoteAddr(uint32_t nSessionId) const
{
  if (nSessionId < m_sessions.size() &&
      m_sessions[nSessionId] != udp::endpoint()) {
    return  m_sessions[nSessionId];
  }
  return std::nullopt;
}

void UdpSocket::attachToTerminal(IBinaryTerminalPtr pTerminal)
{
  m_pTerminal = pTerminal;
}

bool UdpSocket::send(uint32_t nSessionId, const BinaryMessage& message)
{
  std::lock_guard<utils::Mutex> guard(m_Mutex);

  if (nSessionId >= m_sessions.size()) {
    return false;
  }
  udp::endpoint const& remote = m_sessions[nSessionId];
  if (remote == udp::endpoint()) {
    return false;
  }

  uint8_t* pChunk = m_ChunksPool.get(message.m_nLength);
  memcpy(pChunk, message.m_pBody, message.m_nLength);
  m_socket.async_send_to(
        boost::asio::buffer(pChunk, message.m_nLength), remote,
        [this, pChunk](const boost::system::error_code&, std::size_t) {
          m_ChunksPool.release(pChunk);
        });

  if (m_lPromiscMode && nSessionId >= nPersistentSessionsLimit) {
    // In promisc mode, once responce is sent, session should be closed
    m_sessions[nSessionId] = udp::endpoint();
  }
  return true;
}

void UdpSocket::closeSession(uint32_t nSessionId)
{
  if (nSessionId < m_sessions.size()) {
    m_sessions[nSessionId] = udp::endpoint();
  }
}

void UdpSocket::receivingData()
{
  using namespace std::placeholders;
  m_socket.async_receive_from(
        boost::asio::buffer(m_pReceiveBuffer.data(), nReceiveBufferSize),
        m_senderAddress,
        [this](boost::system::error_code const& error, std::size_t nTotalBytes)
        {
          onDataReceived(error, nTotalBytes);
          // Continue receiving data
          receivingData();
        });
}

void UdpSocket::onDataReceived(boost::system::error_code const& error,
                               std::size_t nTotalBytes)
{
  if (!error)
  { 
    std::optional<uint32_t> nSessionId;

    // Linear complicity in searching for sessionId is OK, because in general
    // we won't have a lot of sessions (nSessionsLimit is just 8)
    for(size_t i = 0; i < nPersistentSessionsLimit; ++i) {
      if (m_senderAddress == m_sessions[i]) {
        nSessionId = i;
        break;
      }
    }

    if (nSessionId.has_value()) {  // [[likely]]
      m_pTerminal->onMessageReceived(
              *nSessionId, BinaryMessage(m_pReceiveBuffer.data(), nTotalBytes));
    } else if (m_lPromiscMode) {
      for(size_t i = nPersistentSessionsLimit; i < m_sessions.size(); ++i) {
        // To prevent spamming from the same IP:
        size_t nAlreadyOpened = 0;
        if (m_senderAddress.address() == m_sessions[i].address()) {
          ++nAlreadyOpened;
          if (nAlreadyOpened == nPersistentSessionsLimit) {
            // Too many simultanious requests from the same IP
            return;
          }
        }
        if (!nSessionId && m_sessions[i] == udp::endpoint()) {
          nSessionId = i;
        }
      }

      if (nSessionId.has_value()) {
        m_sessions[*nSessionId] = m_senderAddress;
        m_pTerminal->onMessageReceived(
              *nSessionId, BinaryMessage(m_pReceiveBuffer.data(), nTotalBytes));
      }
    } else {
      std::cerr << "Drop message from " << m_senderAddress << " at " << getLocalAddr() << " (" << this << ")" << std::endl;
    }
  } else {
    assert(nullptr == "unexpected boost.asio error!");
  }
}

} // namespace network

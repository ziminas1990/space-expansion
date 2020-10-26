#include "UdpDispatcher.h"

namespace network {

UdpDispatcher::UdpDispatcher(boost::asio::io_service& ioContext,
                             uint16_t nPoolBegin, uint16_t nPoolEnd)
  : m_IOContext(ioContext),
    m_portsPool(nPoolBegin, nPoolEnd)
{}

UdpSocketPtr UdpDispatcher::createUdpConnection(uint16_t nLocalPort)
{
  std::lock_guard<utils::Mutex> guard(m_Mutex);

  if (!nLocalPort) {
    nLocalPort = m_portsPool.getNext();
    if (!nLocalPort)
      return UdpSocketPtr();
  }
  return std::make_shared<UdpSocket>(m_IOContext, nLocalPort);
}

bool UdpDispatcher::prephare(uint16_t, uint32_t, uint64_t)
{
  while(m_IOContext.poll());
  return false;
}

void UdpDispatcher::proceed(uint16_t, uint32_t, uint64_t)
{
  assert(nullptr == "This function should never be called!");
}

} // namespace network

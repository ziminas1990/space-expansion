#pragma once

#include <boost/asio.hpp>

#include <atomic>
#include <Conveyor/IAbstractLogic.h>
#include <Utils/SimpleIdPool.h>
#include <Utils/Mutex.h>
#include <Network/Fwd.h>

namespace network
{

using UdpEndPoint = boost::asio::ip::udp::endpoint;
using TcpEndPoint = boost::asio::ip::tcp::endpoint;

class UdpDispatcher : public conveyor::IAbstractLogic
{ 
public:
  UdpDispatcher(boost::asio::io_service& ioContext,
                uint16_t nPoolBegin, uint16_t nPoolEnd);

  UdpSocketPtr createUdpSocket(uint16_t nLocalPort = 0, 
                               bool lPromiscMode = false);

  // overrides from IAbstractLogic interface
  uint16_t getStagesCount() override { return 1; }
  bool     prephare(uint16_t nStageId, uint32_t nIntervalUs, uint64_t now) override;
  void     proceed(uint16_t nStageId, uint32_t nIntervalUs, uint64_t) override;

  size_t   getCooldownTimeUs() const override { return 0; }

private:
  boost::asio::io_service& m_IOContext;

  utils::SimpleIdPool<uint16_t, 0> m_portsPool;

  utils::Mutex       m_Mutex;
};

} // namespace network

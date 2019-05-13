#pragma once

#include <boost/asio.hpp>

#include <atomic>
#include <Conveyor/IAbstractLogic.h>
#include <Utils/SimplePool.h>
#include <Utils/Mutex.h>
#include "BufferedTerminal.h"
#include "UdpSocket.h"

namespace network
{

using UdpEndPoint = boost::asio::ip::udp::endpoint;
using TcpEndPoint = boost::asio::ip::tcp::endpoint;

class UdpDispatcher : public conveyor::IAbstractLogic
{ 
public:
  UdpDispatcher(boost::asio::io_service& ioContext);

  UdpSocketPtr createUdpConnection(BufferedTerminalPtr pTerminal,
                                   uint16_t nLocalPort = 0);

  // overrides from IAbstractLogic interface
  uint16_t getStagesCount() override { return 1; }
  bool     prephareStage(uint16_t nStageId) override;
  void     proceedStage(uint16_t nStageId, uint32_t nIntervalUs) override;
  size_t   getCooldownTimeUs() const override { return 3000; }

private:
  void addConnection(IBinaryChannelPtr pChannel, BufferedTerminalPtr pTerminal);

private:
  struct Connection
  {
    Connection(IBinaryChannelPtr pChannel, BufferedTerminalPtr pTerminal)
      : m_pSocket(pChannel), m_pTerminal(pTerminal)
    {}

    IBinaryChannelPtr   m_pSocket;
    BufferedTerminalPtr m_pTerminal;
  };

private:
  boost::asio::io_service& m_IOContext;
  std::vector<Connection>  m_Connections;

  utils::SimplePool<uint16_t, 0> m_portsPool;

  std::atomic_size_t m_nNextConnectionId;
  utils::Mutex       m_Mutex;
};

using UdpDispatcherPtr = std::shared_ptr<UdpDispatcher>;

} // namespace network

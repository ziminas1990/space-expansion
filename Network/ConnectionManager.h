#pragma once

#include <boost/asio.hpp>

#include <atomic>
#include <Conveyor/IAbstractLogic.h>
#include "BufferedTerminal.h"

namespace network
{

class ConnectionManager : public conveyor::IAbstractLogic
{
public:
  ConnectionManager(boost::asio::io_service& ioContext);

  void addConnection(IChannelPtr pChannel, BufferedTerminalPtr pTerminal);

  // overrides from IAbstractLogic interface
  uint16_t getStagesCount() override { return 1; }
  bool     prephareStage(uint16_t nStageId) override;
  void     proceedStage(uint16_t nStageId, size_t nIntervalUs) override;
  size_t   getCooldownTimeUs() const override { return 3000; }

private:
  struct Connection
  {
    Connection(IChannelPtr pChannel, BufferedTerminalPtr pTerminal)
      : m_pSocket(pChannel), m_pTerminal(pTerminal)
    {}

    IChannelPtr         m_pSocket;
    BufferedTerminalPtr m_pTerminal;
  };

private:
  boost::asio::io_service& m_IOContext;
  std::vector<Connection>  m_Connections;
  std::atomic_size_t       m_nNextConnectionId;
};

using ConnectionManagerPtr = std::shared_ptr<ConnectionManager>;

} // namespace network

#pragma once

#include <memory>
#include <atomic>
#include <boost/asio.hpp>

#include <Conveyor/IAbstractLogic.h>
#include "UdpSocket.h"

namespace network
{

class UdpServer : public conveyor::IAbstractLogic
{
public:

  void addHandlerOnPort(uint16_t nPort);

  // from IAbstractLogic interface
  uint16_t getStagesCount() override { return 1; }
  bool prephareStage(uint16_t nStageId) override;
  void proceedStage(uint16_t nStageId, size_t nTicksCount) override;

private:
  boost::asio::io_service    m_IOContext;
  std::vector<UdpSocketUptr> m_Sockets;
  std::atomic_size_t         m_nNextSocketId;
};

using UdpServerUptr = std::unique_ptr<UdpServer>;

} // namespace network

#pragma once

#include "Interfaces.h"

#include <Protocol.pb.h>
#include <Privileged.pb.h>

#include <boost/asio.hpp>

namespace autotests { namespace client {

class Socket
{
public:
  using udp = boost::asio::ip::udp;

  Socket(boost::asio::io_service& io_context, udp::endpoint localAddress);
  Socket(Socket const& other) = delete;
  Socket(Socket&& other)      = delete;
  virtual ~Socket();

  void setServerAddress(udp::endpoint serverAddress);

  void send(std::string&& message);
    // Send the specified 'message' to the server. Why message is
    // 'std::string'? Because protobuf uses std::string

protected:
  virtual void handleData(uint8_t* pData, size_t nTotal) = 0;

private:
  void receivingData();
  void onDataReceived(boost::system::error_code const& error, std::size_t nTotalBytes);

private:
  mutable udp::socket m_socket;
  udp::endpoint       m_senderAddress;
  udp::endpoint       m_serverAddress;

  size_t   m_nReceiveBufferSize;
  uint8_t* m_pReceiveBuffer;
};


template<typename FrameT>
class ProtobufSocket : public Socket,
                       public IChannel<FrameT>
{
public:
  ProtobufSocket(boost::asio::io_service& io_context, udp::endpoint localAddress)
    : Socket(io_context, std::move(localAddress))
  {}

  // overrides from IChannel<FrameT>
  bool send(FrameT&& message) override
  {
    std::string buffer;
    if (!message.SerializeToString(&buffer))
      return false;
    Socket::send(std::move(buffer));
    return true;
  }

  void attachToTerminal(ITerminalPtr<FrameT> pTerminal) override
  {
    m_pTerminalLink = pTerminal;
  }

  void detachFromTerminal() override
  {
    m_pTerminalLink.reset();
  }

protected:
  // override from Socket
  void handleData(uint8_t* pData, size_t nTotal) override
  {
    FrameT receivedMessage;
    if (receivedMessage.ParseFromArray(pData, int(nTotal))) {
      ITerminalPtr<FrameT> pTerminal = m_pTerminalLink.lock();
      if (pTerminal)
        pTerminal->onMessageReceived(std::move(receivedMessage));
    }
  }

private:
  ITerminalWeakPtr<FrameT> m_pTerminalLink;
};


using PlayerSocket = ProtobufSocket<spex::Message>;
using PlayerSocketPtr = std::shared_ptr<PlayerSocket>;

using PrivilegedSocket = ProtobufSocket<admin::Message>;
using PrivilegedSocketPtr = std::shared_ptr<PrivilegedSocket>;

}}  // namespace autotests::client

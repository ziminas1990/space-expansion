#pragma once

#include <memory>
#include "Interfaces.h"
#include "BufferedTerminal.h"

namespace network {

// This class connects UDP socket with protobuf terminal
// It performs converting binary messages into a protobuf messages and vice versa
// The `FrameType` parameter specifies type of protobuf message, like `spex::Message`
// or `admin::Message`
template<typename FrameType>
class ProtobufChannel : public BufferedTerminal, public IChannel<FrameType>
{
  using Terminal = ITerminal<FrameType>;
  using TerminalPtr = std::shared_ptr<Terminal>;
public:
  // from BufferedTerminal->IBinaryTerminal interface:
  bool openSession(uint32_t nSessionId) override;
  void onSessionClosed(uint32_t nSessionId) override;

  // Overrides of IChannel<FrameType> interface
  bool send(uint32_t nSessionId, FrameType const& message) const override;
  void attachToTerminal(TerminalPtr pTerminal) override;
  void detachFromTerminal() override;
  void closeSession(uint32_t nSessionId) override;
  bool isValid() const override;

protected:
  void handleMessage(uint32_t nSessionId, BinaryMessage const& message) override;

private:
  TerminalPtr m_pTerminal;
};

using PlayerChannel = ProtobufChannel<spex::Message>;
using PlayerChannelPtr = std::shared_ptr<PlayerChannel>;

using PrivilegedChannel = ProtobufChannel<admin::Message>;
using PrivilegedChannelPtr = std::shared_ptr<PrivilegedChannel>;


template<typename FrameType>
bool ProtobufChannel<FrameType>::openSession(uint32_t nSessionId)
{
  return m_pTerminal && m_pTerminal->openSession(nSessionId);
}

template<typename FrameType>
void ProtobufChannel<FrameType>::onSessionClosed(uint32_t nSessionId)
{
  if (m_pTerminal)
    m_pTerminal->onSessionClosed(nSessionId);
}

template<typename FrameType>
bool ProtobufChannel<FrameType>::send(uint32_t nSessionId, FrameType const& message) const
{
  std::string buffer;
  message.SerializeToString(&buffer);
  //std::cout << "Sending\n" << message.DebugString() << std::endl;
  return isAttachedToChannel()
      && getChannel()->send(nSessionId, BinaryMessage(buffer.data(), buffer.size()));
}

template<typename FrameType>
void ProtobufChannel<FrameType>::attachToTerminal(TerminalPtr pTerminal)
{
  m_pTerminal = pTerminal;
}

template<typename FrameType>
void ProtobufChannel<FrameType>::detachFromTerminal()
{
  m_pTerminal.reset();
}

template<typename FrameType>
void ProtobufChannel<FrameType>::closeSession(uint32_t nSessionId)
{
  if (isAttachedToChannel())
    getChannel()->closeSession(nSessionId);
}

template<typename FrameType>
bool ProtobufChannel<FrameType>::isValid() const
{
  return isAttachedToChannel() && getChannel()->isValid();
}

template<typename FrameType>
void ProtobufChannel<FrameType>::handleMessage(uint32_t nSessionId,
                                               BinaryMessage const& message)
{
  spex::Message pdu;
  if (pdu.ParseFromArray(message.m_pBody, static_cast<int>(message.m_nLength))) {
    //std::cout << "Received\n" << pdu.DebugString() << std::endl;
    m_pTerminal->onMessageReceived(nSessionId, std::move(pdu));
  }
}

} // namespace network

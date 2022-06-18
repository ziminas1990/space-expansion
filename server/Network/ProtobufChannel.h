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
class ProtobufChannel : public IBinaryTerminal, public IChannel<FrameType>
{
  using Terminal = ITerminal<FrameType>;
  using TerminalPtr = std::shared_ptr<Terminal>;
public:
  // from IBinaryTerminal interface:
  void attachToChannel(IBinaryChannelPtr pChannel) override;
  void detachFromChannel() override;
  bool openSession(uint32_t nSessionId) override;
  void onMessageReceived(uint32_t nSessionId, BinaryMessage const& message) override;
  void onSessionClosed(uint32_t nSessionId) override;

  // Overrides of IChannel<FrameType> interface
  bool send(uint32_t nSessionId, const FrameType& message) override;
  void attachToTerminal(TerminalPtr pTerminal) override;
  void detachFromTerminal() override;
  void closeSession(uint32_t nSessionId) override;
  bool isValid() const override;

private:
  TerminalPtr       m_pTerminal;
  IBinaryChannelPtr m_pChannel;
};

using PlayerChannel = ProtobufChannel<spex::Message>;
using PlayerChannelPtr = std::shared_ptr<PlayerChannel>;

using PrivilegedChannel = ProtobufChannel<admin::Message>;
using PrivilegedChannelPtr = std::shared_ptr<PrivilegedChannel>;


template<typename FrameType>
void ProtobufChannel<FrameType>::attachToChannel(IBinaryChannelPtr pChannel)
{
  m_pChannel = pChannel;
}

template<typename FrameType>
void ProtobufChannel<FrameType>::detachFromChannel()
{
  m_pChannel.reset();
}

template<typename FrameType>
bool ProtobufChannel<FrameType>::openSession(uint32_t nSessionId)
{
  return m_pTerminal && m_pTerminal->openSession(nSessionId);
}

template<typename FrameType>
void ProtobufChannel<FrameType>::onMessageReceived(
    uint32_t nSessionId, BinaryMessage const& message)
{
  FrameType pdu;
  if (pdu.ParseFromArray(message.m_pBody, static_cast<int>(message.m_nLength))) {
    if constexpr (std::is_same_v<FrameType, spex::Message>) {
      std::cerr << "Received\n" << pdu.DebugString() << std::endl;
    }
    m_pTerminal->onMessageReceived(nSessionId, std::move(pdu));
  }
}

template<typename FrameType>
void ProtobufChannel<FrameType>::onSessionClosed(uint32_t nSessionId)
{
  if (m_pTerminal)
    m_pTerminal->onSessionClosed(nSessionId);
}

template<typename FrameType>
bool ProtobufChannel<FrameType>::send(uint32_t nSessionId, FrameType const& message)
{
  std::string buffer;
  message.SerializeToString(&buffer);
  if constexpr (std::is_same_v<FrameType, spex::Message>) {
    std::cerr << "Sending\n" << message.DebugString() << std::endl;
  }
  return m_pChannel
      && m_pChannel->send(nSessionId, BinaryMessage(buffer.data(), buffer.size()));
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
  if (m_pChannel)
    m_pChannel->closeSession(nSessionId);
}

template<typename FrameType>
bool ProtobufChannel<FrameType>::isValid() const
{
  return m_pChannel && m_pChannel->isValid();
}

} // namespace network

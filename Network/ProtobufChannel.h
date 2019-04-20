#pragma once

#include <memory>
#include "Interfaces.h"
#include "BufferedTerminal.h"

namespace network {

class ProtobufChannel : public BufferedTerminal, public IProtobufChannel
{
public:
  void attachToTerminal(IProtobufTerminalPtr pTerminal);
  void attachToChannel(IChannelPtr pChannel);

  // IProtobufChannel interface
  void sendMessage(spex::CommandCenterMessage const& message) override;

protected:
  void handleMessage(MessagePtr pMessage, size_t nLength) override;

private:
  IProtobufTerminalPtr m_pTerminal;
  IChannelPtr          m_pChannel;
};

using ProtobufChannelPtr = std::shared_ptr<ProtobufChannel>;

} // namespace network

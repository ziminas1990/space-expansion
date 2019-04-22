#pragma once

#include <memory>
#include "Interfaces.h"
#include "BufferedTerminal.h"

namespace network {

class ProtobufChannel : public BufferedTerminal, public IProtobufChannel
{
public:
  void attachToTerminal(IProtobufTerminalPtr pTerminal);

  // IProtobufChannel interface
  bool sendMessage(spex::CommandCenterMessage const& message) override;

protected:
  void handleMessage(MessagePtr pMessage, size_t nLength) override;

private:
  IProtobufTerminalPtr m_pTerminal;
};

using ProtobufChannelPtr = std::shared_ptr<ProtobufChannel>;

} // namespace network

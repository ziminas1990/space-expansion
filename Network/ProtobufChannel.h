#pragma once

#include <memory>
#include "Interfaces.h"
#include "BufferedTerminal.h"

namespace network {

class ProtobufChannel : public BufferedTerminal, public IProtobufChannel
{
public:
  // IProtobufChannel interface
  bool sendMessage(size_t nSessionId, spex::CommandCenterMessage const& message) override;
  void attachToTerminal(IProtobufTerminalPtr pTerminal) override;
  void detachFromTerminal() override;

protected:
  void handleMessage(size_t nSessionId, MessagePtr pMessage, size_t nLength) override;

private:
  IProtobufTerminalPtr m_pTerminal;
};

using ProtobufChannelPtr = std::shared_ptr<ProtobufChannel>;

} // namespace network

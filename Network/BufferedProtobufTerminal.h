#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>
#include "ChunksPool.h"

namespace network {

class BufferedProtobufTerminal : public IProtobufTerminal
{
public:
  BufferedProtobufTerminal() { m_messages.reserve(0x40); }

  // overrides from IProtobufTerminal interface
  void onMessageReceived(spex::CommandCenterMessage&& message) override {
    m_messages.push_back(std::move(message));
  }
  void attachToChannel(IProtobufChannelPtr pChannel) override { m_pChannel = pChannel; }
  void detachFromChannel() override { m_pChannel.reset(); }

  void handleBufferedMessages() {
    for(spex::CommandCenterMessage const& message : m_messages)
      handleMessage(message);
    m_messages.clear();
  }

protected:
  virtual void handleMessage(spex::CommandCenterMessage const& message) = 0;
  bool send(spex::CommandCenterMessage const& message) {
    return m_pChannel && m_pChannel->sendMessage(message);
  }

private:
  std::vector<spex::CommandCenterMessage> m_messages;

  IProtobufChannelPtr m_pChannel;
};

using BufferedProtobufTerminalPtr = std::shared_ptr<BufferedProtobufTerminal>;

} // namespace network

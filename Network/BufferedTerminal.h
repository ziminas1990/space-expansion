#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>
#include "ChunksPool.h"

namespace network {

class BufferedTerminal : public ITerminal
{
public:
  BufferedTerminal(size_t nSmallChunksCount  = 512,
                   size_t nMediumChunksCount = 64,
                   size_t nHugeChunksCount   = 16);

  // overrides from ITerminal interface
  void onMessageReceived(MessagePtr pMessage, size_t nLength) override;
  void attachToChannel(IChannelPtr pChannel) override;
  void detachFromChannel() override { m_pChannel.reset(); }

  void handleBufferedMessages();

protected:
  virtual void handleMessage(MessagePtr pMessage, size_t nLength) = 0;
  bool send(MessagePtr pMessage, size_t nLength) {
    return m_pChannel && m_pChannel->sendMessage(pMessage, nLength);
  }

private:
  struct BufferedMessage
  {
    uint8_t* m_pBody       = nullptr;
    size_t   m_nLength = 0;
  };

private:
  ChunksPool m_ChunksPool;
  std::vector<BufferedMessage> m_messages;

  IChannelPtr m_pChannel;
};

using BufferedTerminalPtr = std::shared_ptr<BufferedTerminal>;


class IBufferedTerminalFactory
{
public:
  virtual ~IBufferedTerminalFactory() = default;

  virtual BufferedTerminalPtr make() = 0;
};

using IBufferedTerminalFactoryPtr     = std::shared_ptr<IBufferedTerminalFactory>;
using IBufferedTerminalFactoryWeakPtr = std::weak_ptr<IBufferedTerminalFactory>;

} // namespace network

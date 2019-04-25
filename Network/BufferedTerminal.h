#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>
#include <Utils/ChunksPool.h>

namespace network {

class BufferedTerminal : public ITerminal
{
public:
  BufferedTerminal(size_t nSmallChunksCount  = 512,
                   size_t nMediumChunksCount = 64,
                   size_t nHugeChunksCount   = 16);

  // overrides from ITerminal interface
  void onMessageReceived(size_t nSessionId, MessagePtr pMessage, size_t nLength) override;
  void attachToChannel(IChannelPtr pChannel) override;
  void detachFromChannel() override { m_pChannel.reset(); }

  void handleBufferedMessages();

protected:
  virtual void handleMessage(size_t nSessionId, MessagePtr pMessage, size_t nLength) = 0;
  bool send(size_t nSessionId, MessagePtr pMessage, size_t nLength) {
    return m_pChannel && m_pChannel->sendMessage(nSessionId, pMessage, nLength);
  }
  void closeSession(size_t nSessionId);

private:
  struct BufferedMessage
  {
    size_t   m_nSessionId = 0;
    uint8_t* m_pBody    = nullptr;
    size_t   m_nLength  = 0;
  };

private:
  utils::ChunksPool m_ChunksPool;
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

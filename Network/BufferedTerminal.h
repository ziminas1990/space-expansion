#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>
#include <Utils/ChunksPool.h>

namespace network {

// When subclassing this class, you MUST override:
// 1. IBinaryTerminal::openSession(sessionId)
// 2. IBinaryTerminal::onSessionClosed(sessionId)
class BufferedTerminal : public IBinaryTerminal
{
public:
  BufferedTerminal(size_t nSmallChunksCount  = 512,
                   size_t nMediumChunksCount = 64,
                   size_t nHugeChunksCount   = 16);

  // overrides from ITerminal interface
  void onMessageReceived(uint32_t nSessionId, BinaryMessage&& body) override;
  void attachToChannel(IBinaryChannelPtr pChannel) override;
  void detachFromChannel() override { m_pChannel.reset(); }

  void handleBufferedMessages();

protected:
  virtual void handleMessage(uint32_t nSessionId, BinaryMessage const& message) = 0;

  bool isAttachedToChannel() const { return m_pChannel->isValid(); }
  IBinaryChannelPtr const& getChannel() const { return m_pChannel; }
  IBinaryChannelPtr&       getChannel()       { return m_pChannel; }

private:
  struct BufferedMessage
  {
    uint32_t m_nSessionId = 0;
    uint8_t* m_pBody      = nullptr;
    size_t   m_nLength    = 0;
  };

private:
  utils::ChunksPool m_ChunksPool;
  std::vector<BufferedMessage> m_messages;

  IBinaryChannelPtr m_pChannel;
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

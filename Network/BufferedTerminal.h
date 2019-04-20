#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>
#include "ChunksPool.h"

namespace network {

class BufferedTerminal : public ITerminal
{
public:
  BufferedTerminal();

  void onMessageReceived(MessagePtr pMessage, size_t nLength) override;

  void handleBufferedMessages();

protected:
  virtual void handleMessage(MessagePtr pMessage, size_t nLength) = 0;

private:
  struct BufferedMessage
  {
    uint8_t* m_pBody       = nullptr;
    size_t   m_nLength = 0;
  };

private:
  ChunksPool m_ChunksPool;
  std::vector<BufferedMessage> m_messages;
};

using BufferedTerminalPtr     = std::shared_ptr<BufferedTerminal>;

} // namespace network

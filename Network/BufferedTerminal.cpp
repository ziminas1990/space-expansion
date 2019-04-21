#include "BufferedTerminal.h"

namespace network {


BufferedTerminal::BufferedTerminal(size_t nSmallChunksCount,
                                   size_t nMediumChunksCount,
                                   size_t nHugeChunksCount)
  : m_ChunksPool(nSmallChunksCount, nMediumChunksCount, nHugeChunksCount)
{
  m_messages.reserve(0xFF);
}

void BufferedTerminal::onMessageReceived(MessagePtr pMessage, size_t nLength)
{
  BufferedMessage message;
  message.m_pBody = m_ChunksPool.get(nLength);
  if (message.m_pBody == nullptr)
    message.m_pBody = new uint8_t[nLength];
  message.m_nLength = nLength;
  memcpy(message.m_pBody, pMessage, nLength);

  m_messages.push_back(std::move(message));
}

void BufferedTerminal::attachToChannel(IChannelPtr pChannel)
{
  m_pChannel = pChannel;
}

void BufferedTerminal::handleBufferedMessages()
{
  for(BufferedMessage& message : m_messages) {
    handleMessage(message.m_pBody, message.m_nLength);
    if (!m_ChunksPool.release(message.m_pBody))
      delete [] message.m_pBody;
  }
  m_messages.clear();
}

} // namespace network

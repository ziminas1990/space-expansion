#include "BufferedTerminal.h"

namespace network {

BufferedTerminal::BufferedTerminal(size_t nSmallChunksCount,
                                   size_t nMediumChunksCount,
                                   size_t nHugeChunksCount)
  : m_ChunksPool(nSmallChunksCount, nMediumChunksCount, nHugeChunksCount)
{
  m_messages.reserve(0xFF);
}

void BufferedTerminal::onMessageReceived(uint32_t nSessionId, BinaryMessage const& body)
{
  uint8_t* pCopiedBody = m_ChunksPool.get(body.m_nLength);
  if (!pCopiedBody)
    pCopiedBody = new uint8_t[body.m_nLength];
  memcpy(pCopiedBody, body.m_pBody, body.m_nLength);
  m_messages.emplace_back(nSessionId, pCopiedBody, body.m_nLength);
}

void BufferedTerminal::attachToChannel(IBinaryChannelPtr pChannel)
{
  m_pChannel = pChannel;
}

void BufferedTerminal::handleBufferedMessages()
{
  for(BufferedMessage& message : m_messages) {
    handleMessage(message.m_nSessionId,
                  BinaryMessage(message.m_pBody, message.m_nLength));
    if (!m_ChunksPool.release(message.m_pBody))
      delete [] message.m_pBody;
  }
  m_messages.clear();
}

} // namespace network

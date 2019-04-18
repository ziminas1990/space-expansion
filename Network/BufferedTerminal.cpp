#include "BufferedTerminal.h"

namespace network {

BufferedTerminal::BufferedTerminal(size_t nCapacity) :
  m_nBytesReceived(0),
  m_nBytesLeft(nCapacity),
  m_pBuffer(new uint8_t[nCapacity])
{
  m_messagesLengths.reserve(0xFF);
}

BufferedTerminal::~BufferedTerminal()
{
  delete [] m_pBuffer;
}

void BufferedTerminal::onMessageReceived(MessagePtr pMessage, size_t nLength)
{
  if (!ableToSave(nLength))
    return;
  memcpy(front(), pMessage, nLength);
  onMessagePushed(nLength);
}

void BufferedTerminal::handleBufferedMessages()
{
  uint8_t* pMessage = m_pBuffer;
  for (size_t nLength : m_messagesLengths) {
    handleMessage(pMessage, nLength);
    pMessage += nLength;
  }
  m_messagesLengths.clear();
  m_nBytesLeft     += m_nBytesReceived;
  m_nBytesReceived  = 0;
}

void BufferedTerminal::onMessagePushed(size_t nLength)
{
  m_nBytesReceived += nLength;
  m_nBytesLeft     -= nLength;
  m_messagesLengths.push_back(nLength);
}

} // namespace network

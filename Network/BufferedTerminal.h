#pragma once

#include "Interfaces.h"
#include <functional>
#include <vector>

namespace network {

class BufferedTerminal : public ITerminal
{
public:
  BufferedTerminal(size_t nCapacity);
  ~BufferedTerminal() override;

  void onMessageReceived(MessagePtr pMessage, size_t nLength) override;

  void handleBufferedMessages();

protected:
  virtual void handleMessage(MessagePtr pMessage, size_t nLength) = 0;

private:
  bool ableToSave(size_t nLength) const { return nLength <= m_nBytesLeft; }
  void onMessagePushed(size_t nLength);

  uint8_t* front()                { return m_pBuffer + m_nBytesReceived; }
  size_t   bytesAvaliable() const { return m_nBytesLeft; }

private:
  size_t   m_nBytesReceived;
  size_t   m_nBytesLeft;
  uint8_t* m_pBuffer;
  std::vector<size_t> m_messagesLengths;
};


} // namespace network

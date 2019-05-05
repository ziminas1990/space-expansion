#include "ProtobufSyncPipe.h"
#include <Utils/WaitingFor.h>

namespace autotests {

bool ProtobufSyncPipe::waitAny(
    uint32_t nSessionId, spex::ICommutator &out, uint16_t nTimeoutMs)
{
  if (m_knownSessionsIds.find(nSessionId) == m_knownSessionsIds.end())
    return false;

  std::function<bool()> fPredicate = [this, nSessionId]() {
    auto itSession = m_Sessions.find(nSessionId);
    return itSession != m_Sessions.end() && !itSession->second.empty();
  };
  if (!utils::waitFor(fPredicate, m_fEnviromentProceeder, nTimeoutMs))
    return false;

  auto& sessionQueue = m_Sessions[nSessionId];
  out = std::move(sessionQueue.front());
  sessionQueue.pop();
  return true;
}

bool ProtobufSyncPipe::waitAny(
    uint32_t* pNewSessionId, spex::ICommutator &out, uint16_t nTimeoutMs)
{
  std::function<bool()> fPredicate = [this]() { return m_newSessionIds.empty(); };
  if (!utils::waitFor(fPredicate, m_fEnviromentProceeder, nTimeoutMs))
    return false;

  uint32_t nNewSessionId = *m_newSessionIds.begin();
  m_newSessionIds.erase(m_newSessionIds.begin());
  m_knownSessionsIds.insert(nNewSessionId);
  if (pNewSessionId)
    *pNewSessionId = nNewSessionId;

  auto& sessionQueue = m_Sessions[nNewSessionId];
  out = std::move(sessionQueue.front());
  sessionQueue.pop();
  return true;
}

bool ProtobufSyncPipe::expectSilence(uint32_t nSessionId, uint16_t nTimeoutMs)
{
  spex::ICommutator message;
  return !waitAny(nSessionId, message, nTimeoutMs);
}

void ProtobufSyncPipe::onMessageReceived(uint32_t nSessionId, spex::ICommutator &&frame)
{
  if (m_eMode == eMockedTerminalMode) {
    storeMessage(nSessionId, std::move(frame));
  } else {
    m_pAttachedTerminal->onMessageReceived(nSessionId, std::move(frame));
    m_knownSessionsIds.insert(nSessionId);
  }
}

bool ProtobufSyncPipe::send(uint32_t nSessionId, spex::ICommutator &&frame) const
{
  if (m_eMode == eMockedChannelMode) {
    storeMessage(nSessionId, std::move(frame));
  } else {
    m_pAttachedChannel->send(nSessionId, std::move(frame));
    m_knownSessionsIds.insert(nSessionId);
  }
  return true;
}

void ProtobufSyncPipe::storeMessage(
    uint32_t nSessionId, spex::ICommutator &&message) const
{
  if (m_Sessions.find(nSessionId) == m_Sessions.end()) {
    m_newSessionIds.insert(nSessionId);
  }
  m_Sessions[nSessionId].push(std::move(message));
}

} // namespace autotest

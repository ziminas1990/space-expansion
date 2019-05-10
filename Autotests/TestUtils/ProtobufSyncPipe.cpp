#include "ProtobufSyncPipe.h"
#include <Utils/WaitingFor.h>

namespace autotests {

void ProtobufSyncPipe::attachToTunnel(
    uint32_t nSessionId, network::IProtobufTerminalPtr pUplevel)
{
  m_clientTunnels[nSessionId] = pUplevel;
}

bool ProtobufSyncPipe::waitAny(uint32_t nSessionId, uint16_t nTimeoutMs)
{
  std::function<bool()> fPredicate = [this, nSessionId]() {
    auto itSession = m_Sessions.find(nSessionId);
    return itSession != m_Sessions.end() && !itSession->second.empty();
  };
  return utils::waitFor(fPredicate, m_fEnviromentProceeder, nTimeoutMs);
}

bool ProtobufSyncPipe::waitAny(
    uint32_t nSessionId, spex::Message &out, uint16_t nTimeoutMs)
{
  if (!waitAny(nSessionId, nTimeoutMs))
    return false;
  auto& sessionQueue = m_Sessions[nSessionId];
  out = std::move(sessionQueue.front());
  sessionQueue.pop();
  return true;
}

bool ProtobufSyncPipe::wait(uint32_t nSessionId, spex::ICommutator &out,
                            uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(nSessionId, spex::Message::kCommutator, message, nTimeoutMs))
    return false;
  out = std::move(message.commutator());
  return true;
}

bool ProtobufSyncPipe::wait(uint32_t nSessionId, spex::INavigation &out,
                            uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(nSessionId, spex::Message::kNavigation, message, nTimeoutMs))
    return false;
  out = std::move(message.navigation());
  return true;
}

bool ProtobufSyncPipe::expectSilence(uint32_t nSessionId, uint16_t nTimeoutMs)
{
  spex::Message message;
  return !waitAny(nSessionId, message, nTimeoutMs);
}

bool ProtobufSyncPipe::openSession(uint32_t nSessionId)
{
  return m_pAttachedTerminal && m_pAttachedTerminal->openSession(nSessionId);
}

void ProtobufSyncPipe::onMessageReceived(uint32_t nSessionId, spex::Message const& message)
{
  if (message.choice_case() == spex::Message::kEncapsulated) {
    auto itTunnel = m_clientTunnels.find(nSessionId);
    if (itTunnel != m_clientTunnels.end()) {
      itTunnel->second->onMessageReceived(message.tunnelid(), message.encapsulated());
    }
  } else {
    storeMessage(nSessionId, message);
  }
}

void ProtobufSyncPipe::onSessionClosed(uint32_t nSessionId)
{
  if (m_pAttachedTerminal)
    m_pAttachedTerminal->onSessionClosed(nSessionId);
}

bool ProtobufSyncPipe::send(uint32_t nSessionId, spex::Message const& message) const
{
  m_knownSessionsIds.insert(nSessionId);
  return m_pAttachedChannel->send(nSessionId, message);
}

void ProtobufSyncPipe::closeSession(uint32_t nSessionId)
{
  if (m_pAttachedChannel)
    m_pAttachedChannel->closeSession(nSessionId);
}

void ProtobufSyncPipe::storeMessage(
    uint32_t nSessionId, spex::Message const& message) const
{
  if (m_knownSessionsIds.find(nSessionId) == m_knownSessionsIds.end()) {
    m_newSessionIds.insert(nSessionId);
  }
  m_Sessions[nSessionId].push(message);
}

bool ProtobufSyncPipe::waitConcrete(
    uint32_t nSessionId, spex::Message::ChoiceCase eExpectedChoice,
    spex::Message &out, uint16_t nTimeoutMs)
{
  return waitAny(nSessionId, out, nTimeoutMs) && out.choice_case() == eExpectedChoice;
}

} // namespace autotest

#include "ProtobufSyncPipe.h"
#include <Utils/WaitingFor.h>

namespace autotests {

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

bool ProtobufSyncPipe::wait(uint32_t nSessionId, spex::IAccessPanel &out,
                            uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(nSessionId, spex::Message::kAccessPanel, message, nTimeoutMs))
    return false;
  out = std::move(message.accesspanel());
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

void ProtobufSyncPipe::onMessageReceived(uint32_t nSessionId, spex::Message const& message)
{
  m_Sessions[nSessionId].push(message);
}

bool ProtobufSyncPipe::waitConcrete(
    uint32_t nSessionId, spex::Message::ChoiceCase eExpectedChoice,
    spex::Message &out, uint16_t nTimeoutMs)
{
  return waitAny(nSessionId, out, nTimeoutMs) && out.choice_case() == eExpectedChoice;
}

} // namespace autotest

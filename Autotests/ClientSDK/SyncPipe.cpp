#include "SyncPipe.h"

#include <Utils/WaitingFor.h>

namespace autotests { namespace client {

void SyncPipe::attachToDownlevel(IClientChannelPtr pDownlevel)
{
  m_pDownlevel = pDownlevel;
}

void SyncPipe::attachTunnelHandler(uint32_t nTunnelId, IClientTerminalWeakPtr pHandler)
{
  m_handlers[nTunnelId] = pHandler;
}

void SyncPipe::setProceeder(std::function<void ()> fEnviromentProceeder)
{
  m_fEnviromentProceeder = std::move(fEnviromentProceeder);
}

bool SyncPipe::waitAny(uint16_t nTimeoutMs)
{
  return utils::waitFor(
        [this]() { return !m_receivedMessages.empty(); },
        m_fEnviromentProceeder, nTimeoutMs);
}

bool SyncPipe::waitAny(spex::Message &out, uint16_t nTimeoutMs)
{
  if (!waitAny(nTimeoutMs))
    return false;
  out = std::move(m_receivedMessages.front());
  m_receivedMessages.pop();
  return true;
}

bool SyncPipe::wait(spex::IAccessPanel &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kAccessPanel, message, nTimeoutMs))
    return false;
  out = std::move(message.accesspanel());
  return true;
}

bool SyncPipe::wait(spex::ICommutator &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kCommutator, message, nTimeoutMs))
    return false;
  out = std::move(message.commutator());
  return true;
}

bool SyncPipe::wait(spex::INavigation &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kNavigation, message, nTimeoutMs))
    return false;
  out = std::move(message.navigation());
  return true;
}

bool SyncPipe::wait(spex::IEngine &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kEngine, message, nTimeoutMs))
    return false;
  out = std::move(message.engine());
  return true;
}


void SyncPipe::onMessageReceived(spex::Message &&message)
{
  switch(message.choice_case()) {
    case spex::Message::kEncapsulated: {
      spex::Message encapsulated = message.encapsulated();
      auto I = m_handlers.find(message.tunnelid());
      if (I == m_handlers.end()) {
        return;
      }
      IClientTerminalPtr pHandler = I->second.lock();
      if (pHandler) {
        pHandler->onMessageReceived(std::move(encapsulated));
      } else {
        m_handlers.erase(I);
      }
      return;
    }
    default: {
      m_receivedMessages.push(std::move(message));
    }
  }
}

bool SyncPipe::send(spex::Message const& message)
{
  return m_pDownlevel && m_pDownlevel->send(message);
}

bool SyncPipe::waitConcrete(spex::Message::ChoiceCase eExpectedChoice,
                            spex::Message &out, uint16_t nTimeoutMs)
{
  return waitAny(out, nTimeoutMs) && out.choice_case() == eExpectedChoice;
}

bool Tunnel::send(spex::Message const& body)
{
  spex::Message message;
  message.set_tunnelid(m_nTunnelId);
  *message.mutable_encapsulated() = body;
  return SyncPipe::send(message);
}

}}  // namespace autotests::client

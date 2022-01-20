#include "SyncPipe.h"

#include <Utils/WaitingFor.h>

namespace autotests { namespace client {

//==============================================================================
// PlayerPipe
//==============================================================================

void PlayerPipe::attachTunnelHandler(uint32_t nTunnelId,
                                     IPlayerTerminalWeakPtr pHandler)
{
  m_handlers[nTunnelId] = pHandler;
}

void PlayerPipe::onMessageReceived(spex::Message &&message)
{
  switch(message.choice_case()) {
    case spex::Message::kEncapsulated: {
      spex::Message encapsulated = message.encapsulated();
      auto I = m_handlers.find(message.tunnelid());
      if (I == m_handlers.end()) {
        return;
      }
      IPlayerTerminalPtr pHandler = I->second.lock();
      if (pHandler) {
        pHandler->onMessageReceived(std::move(encapsulated));
      } else {
        m_handlers.erase(I);
      }
      return;
    }
    default: {
      SyncPipe<spex::Message>::onMessageReceived(std::move(message));
    }
  }
}

bool PlayerPipe::waitConcrete(spex::Message::ChoiceCase eExpectedChoice,
                              spex::Message &out,
                              uint16_t nTimeoutMs)
{
  return waitAny(out, nTimeoutMs) && out.choice_case() == eExpectedChoice;
}

bool PlayerPipe::pickConcrete(spex::Message::ChoiceCase eExpectedChoice,
                              spex::Message &out)
{
  return pickAny(out) && out.choice_case() == eExpectedChoice;
}

//==============================================================================
// Tunnel
//==============================================================================

bool Tunnel::send(spex::Message const& body)
{
  spex::Message message;
  message.set_tunnelid(m_nTunnelId);
  *message.mutable_encapsulated() = body;
  return PlayerPipe::send(message);
}

//==============================================================================
// PrivilegedPipe
//==============================================================================

bool PrivilegedPipe::wait(admin::Access &out, uint16_t nTimeoutMs)
{
  admin::Message message;
  if(!waitConcrete(admin::Message::kAccess, message, nTimeoutMs))
    return false;
  out = std::move(message.access());
  return true;
}

bool PrivilegedPipe::wait(admin::SystemClock &out, uint16_t nTimeoutMs)
{
  admin::Message message;
  if(!waitConcrete(admin::Message::kSystemClock, message, nTimeoutMs))
    return false;
  out = std::move(message.system_clock());
  return true;
}

bool PrivilegedPipe::waitConcrete(admin::Message::ChoiceCase eExpectedChoice,
                                  admin::Message &out, uint16_t nTimeoutMs)
{
  return waitAny(out, nTimeoutMs) && out.choice_case() == eExpectedChoice;
}

}}  // namespace autotests::client

#include "SyncPipe.h"

#include <Utils/WaitingFor.h>

namespace autotests { namespace client {

//==============================================================================
// PlayerPipe
//==============================================================================

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

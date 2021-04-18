#include "SyncPipe.h"

#include <Utils/WaitingFor.h>

namespace autotests { namespace client {

//========================================================================================
// PlayerPipe
//========================================================================================

void PlayerPipe::attachTunnelHandler(uint32_t nTunnelId, IPlayerTerminalWeakPtr pHandler)
{
  m_handlers[nTunnelId] = pHandler;
}

bool PlayerPipe::wait(spex::IAccessPanel &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kAccessPanel, message, nTimeoutMs))
    return false;
  out = std::move(message.accesspanel());
  return true;
}

bool PlayerPipe::wait(spex::ICommutator &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kCommutator, message, nTimeoutMs))
    return false;
  out = std::move(message.commutator());
  return true;
}

bool PlayerPipe::wait(spex::IShip &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kShip, message, nTimeoutMs))
    return false;
  out = std::move(message.ship());
  return true;
}

bool PlayerPipe::wait(spex::INavigation &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kNavigation, message, nTimeoutMs))
    return false;
  out = std::move(message.navigation());
  return true;
}

bool PlayerPipe::wait(spex::IEngine &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kEngine, message, nTimeoutMs))
    return false;
  out = std::move(message.engine());
  return true;
}

bool PlayerPipe::wait(spex::ICelestialScanner &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kCelestialScanner, message, nTimeoutMs))
    return false;
  out = std::move(message.celestial_scanner());
  return true;
}

bool PlayerPipe::wait(spex::IAsteroidScanner &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kAsteroidScanner, message, nTimeoutMs))
    return false;
  out = std::move(message.asteroid_scanner());
  return true;
}

bool PlayerPipe::wait(spex::IResourceContainer &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kResourceContainer, message, nTimeoutMs))
    return false;
  out = std::move(message.resource_container());
  return true;
}

bool PlayerPipe::wait(spex::IAsteroidMiner &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kAsteroidMiner, message, nTimeoutMs))
    return false;
  out = std::move(message.asteroid_miner());
  return true;
}

bool PlayerPipe::wait(spex::IBlueprintsLibrary &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kBlueprintsLibrary, message, nTimeoutMs))
    return false;
  out = std::move(message.blueprints_library());
  return true;
}

bool PlayerPipe::wait(spex::IShipyard &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kShipyard, message, nTimeoutMs))
    return false;
  out = std::move(message.shipyard());
  return true;
}

bool PlayerPipe::wait(spex::IGame& out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kGame, message, nTimeoutMs))
    return false;
  out = std::move(message.game());
  return true;
}

bool PlayerPipe::wait(spex::IMonitor &out, uint16_t nTimeoutMs)
{
  spex::Message message;
  if(!waitConcrete(spex::Message::kMonitor, message, nTimeoutMs)) {
    std::cerr << "Unexpected message: " << message.DebugString();
    return false;
  }
  out = std::move(message.monitor());
  return true;
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
                              spex::Message &out, uint16_t nTimeoutMs)
{
  return waitAny(out, nTimeoutMs) && out.choice_case() == eExpectedChoice;
}

//========================================================================================
// Tunnel
//========================================================================================

bool Tunnel::send(spex::Message const& body)
{
  spex::Message message;
  message.set_tunnelid(m_nTunnelId);
  *message.mutable_encapsulated() = body;
  return PlayerPipe::send(message);
}

//========================================================================================
// PrivilegedPipe
//========================================================================================

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

#include "SystemClock.h"

#include <SystemManager.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::SystemClock);

namespace modules {

SystemClock::SystemClock(std::string&& sName, world::PlayerWeakPtr pOwner)
  : BaseModule ("SystemClock", std::move(sName), std::move(pOwner))
{
  GlobalContainer<SystemClock>::registerSelf(this);
}

void SystemClock::proceed(uint32_t)
{
  uint64_t const now = SystemManager::getIngameTime();
  while (!m_rings.empty()) {
    if (m_rings.back().nWhen <= now) {
      sendRing(m_rings.back().nSessionId, now);
      m_rings.pop_back();
    } else {
      break;
    }
  }

  if (m_rings.empty()) {
    switchToIdleState();
  }
}

void SystemClock::handleSystemClockMessage(
    uint32_t nSessionId,
    spex::ISystemClock const& message)
{
  switch (message.choice_case()) {
    case spex::ISystemClock::kTimeReq:
      sendTime(nSessionId);
      return;
    case spex::ISystemClock::kWaitFor:
      waitFor(nSessionId, message.wait_for());
      return;
    case spex::ISystemClock::kWaitUntil:
      waitUntil(nSessionId, message.wait_until());
      return;
    default:
      return;
  }
}

void SystemClock::waitFor(uint32_t nSessionId, uint64_t time)
{
  m_rings.emplace_back(nSessionId, SystemManager::getIngameTime() + time);
  drawnLastRing();
  switchToActiveState();
}

void SystemClock::waitUntil(uint32_t nSessionId, uint64_t time)
{
  m_rings.emplace_back(nSessionId, time);
  drawnLastRing();
  switchToActiveState();
}

void SystemClock::sendTime(uint32_t nSessionId)
{
  spex::Message message;
  spex::ISystemClock* body = message.mutable_system_clock();
  body->set_time(SystemManager::getIngameTime());
  sendToClient(nSessionId, message);
}

void SystemClock::sendRing(uint32_t nSessionId, uint64_t time)
{
  spex::Message message;
  spex::ISystemClock* body = message.mutable_system_clock();
  body->set_ring(time);
  sendToClient(nSessionId, message);
}

void SystemClock::drawnLastRing()
{
  size_t const length = m_rings.size();
  for (size_t i = length - 1; i > 0; --i) {
    if (m_rings[i-1].nWhen > m_rings[i].nWhen) {
      break;
    }
    std::swap(m_rings[i-1], m_rings[i]);
  }
}

} // namespace modules

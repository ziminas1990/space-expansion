#include "SystemClock.h"

#include <Utils/Clock.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::SystemClock);

namespace modules {

SystemClock::SystemClock(std::string&& sName, world::PlayerWeakPtr pOwner)
  : BaseModule ("SystemClock", std::move(sName), std::move(pOwner))
{
  GlobalObject<SystemClock>::registerSelf(this);
}

void SystemClock::proceed(uint32_t)
{
  const uint64_t now = utils::GlobalClock::now();
  while (!m_rings.empty()) {
    if (!m_rings.back().isValid()) {
      m_rings.pop_back();
    } else if (m_rings.back().nWhen <= now) {
      sendRing(m_rings.back().nSessionId, now);
      m_rings.pop_back();
    } else {
      break;
    }
  }

  uint32_t nSessionId = 0;
  while (m_subscriptions.nextUpdate(nSessionId, now)) {
    sendTime(nSessionId);
  }

  if (m_rings.empty() && m_subscriptions.total() == 0) {
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
    case spex::ISystemClock::kMonitor:
      monitoring(nSessionId, message.monitor());
      return;
    default:
      return;
  }
}

void SystemClock::onSessionClosed(uint32_t nSessionId)
{
  m_subscriptions.remove(nSessionId);
  forgetRingSession(nSessionId);
}

void SystemClock::waitFor(uint32_t nSessionId, uint64_t time)
{
  m_rings.emplace_back(nSessionId, utils::GlobalClock::now() + time);
  drawnLastRing();
  switchToActiveState();
}

void SystemClock::waitUntil(uint32_t nSessionId, uint64_t time)
{
  m_rings.emplace_back(nSessionId, time);
  drawnLastRing();
  switchToActiveState();
}

void SystemClock::monitoring(uint32_t nSessionId, uint32_t nIntervalMs)
{
  if (nIntervalMs < 20) {
    nIntervalMs = 20;
  }
  m_subscriptions.add(nSessionId, nIntervalMs, utils::GlobalClock::now());
  switchToActiveState();
}

void SystemClock::sendTime(uint32_t nSessionId)
{
  spex::Message message;
  spex::ISystemClock* body = message.mutable_system_clock();
  body->set_time(utils::GlobalClock::now());
  sendToClient(nSessionId, std::move(message));
}

void SystemClock::sendRing(uint32_t nSessionId, uint64_t time)
{
  spex::Message message;
  spex::ISystemClock* body = message.mutable_system_clock();
  body->set_ring(time);
  sendToClient(nSessionId, std::move(message));
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

void SystemClock::forgetRingSession(uint32_t nSessionId)
{
  for (size_t i = 0; i < m_rings.size(); ++i) {
    if (m_rings[i].nSessionId == nSessionId) {
      m_rings[i] = Ring();
      break;
    }
  }
}

} // namespace modules

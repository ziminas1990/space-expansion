#include "SystemClock.h"

#include <SystemManager.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::SystemClock);

namespace modules {

SystemClock::SystemClock(std::string&& sName, world::PlayerWeakPtr pOwner)
  : BaseModule ("SystemClock", std::move(sName), std::move(pOwner)),
    m_nLastCycleTime(0)
{
  GlobalContainer<SystemClock>::registerSelf(this);
}

void SystemClock::proceed(uint32_t)
{
  const uint64_t generatorCycle = 20000;

  uint64_t const now = SystemManager::getIngameTime();
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

  uint32_t cyclesPassed = static_cast<uint32_t>(
        (now - m_nLastCycleTime) / generatorCycle);
  if (cyclesPassed) {
    m_nLastCycleTime += cyclesPassed * generatorCycle;
    if (m_nLastCycleTime < now)
      m_nLastCycleTime = now;
    for (uint32_t session: m_generatorSessions) {
      sendTime(session);
    }
  }

  if (m_rings.empty() && m_generatorSessions.empty()) {
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
    case spex::ISystemClock::kAttachGenerator:
      attachGenerator(nSessionId);
      return;
    case spex::ISystemClock::kDetachGenerator:
      detachGenerator(nSessionId);
      return;
    default:
      return;
  }
}

void SystemClock::onSessionClosed(uint32_t nSessionId)
{
  forgetGeneratorSession(nSessionId);
  forgetRingSession(nSessionId);
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

void SystemClock::attachGenerator(uint32_t nSessionId)
{
  for (uint32_t nSession: m_generatorSessions) {
    if (nSessionId == nSession) {
      // Already attached
      sendGeneratorStatus(nSessionId, spex::ISystemClock::GENERATOR_ATTACHED);
      return;
    }
  }
  m_generatorSessions.push_back(nSessionId);
  if (m_generatorSessions.size() == 1) {
    switchToActiveState();
  }
  sendGeneratorStatus(nSessionId, spex::ISystemClock::GENERATOR_ATTACHED);
}

void SystemClock::detachGenerator(uint32_t nSessionId)
{
  forgetGeneratorSession(nSessionId);
  sendGeneratorStatus(nSessionId, spex::ISystemClock::GENERATOR_DETACHED);
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

void SystemClock::sendGeneratorStatus(uint32_t nSessionId,
                                      spex::ISystemClock::Status eStatus)
{
  spex::Message message;
  spex::ISystemClock* body = message.mutable_system_clock();
  body->set_generator_status(eStatus);
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

void SystemClock::forgetGeneratorSession(uint32_t nSessionId)
{
  for (size_t i = 0; i < m_generatorSessions.size(); ++i) {
    if (m_generatorSessions[i] == nSessionId) {
      m_generatorSessions[i] = m_generatorSessions.back();
      m_generatorSessions.pop_back();
      break;
    }
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

#include "ClockControl.h"

#include <Utils/Clock.h>
#include <SystemManager.h>

namespace administrator {

void ClockControl::proceed()
{
  // The most likely condition goes first
  if (m_nDebugSessionId == network::gInvalidSessionId) {
    return;
  }
  if (m_pSystemManager->getClock().isDebugInProgress()) {
    return;
  }
  sendNow(m_nDebugSessionId);
  m_nDebugSessionId = network::gInvalidSessionId;
}

void ClockControl::handleMessage(uint32_t nSessionId, admin::SystemClock const& message)
{
  switch(message.choice_case()) {
    case admin::SystemClock::kNow:
      sendNow(nSessionId);
      return;
    case admin::SystemClock::kModeReq:
      sendClockStatus(nSessionId);
      return;
    case admin::SystemClock::kSwitchToRealTime:
      clock().switchToRealtimeMode();
      sendClockStatus(nSessionId);
      return;
    case admin::SystemClock::kSwitchToDebugMode:
      clock().switchToDebugMode();
      sendClockStatus(nSessionId);
      return;
    case admin::SystemClock::kTerminate:
      clock().terminate();
      sendClockStatus(nSessionId);
      if (m_nDebugSessionId != network::gInvalidSessionId) {
        sendClockStatus(m_nDebugSessionId);
      }
      return;
    case admin::SystemClock::kTickDurationUs:
      if (clock().isDebugInProgress()) {
        sendStatus(nSessionId, admin::SystemClock::CLOCK_IS_BUSY);
      } else {
        clock().setDebugTickUs(message.tick_duration_us());
        sendClockStatus(nSessionId);
      }
      return;
    case admin::SystemClock::kProceedTicks:
      if (clock().proceedRequest(message.proceed_ticks())) {
        m_nDebugSessionId = nSessionId;
      } else {
        sendStatus(nSessionId, admin::SystemClock::CLOCK_IS_BUSY);
      }
      return;
    default:
      assert("Unexpected message" == nullptr);
  }
}

utils::Clock& ClockControl::clock()
{
  return m_pSystemManager->getClock();
}

void ClockControl::onSwtichToRealTimeReq(uint32_t nSessionId)
{
  utils::Clock& clock = m_pSystemManager->getClock();
  if (clock.isInRealTimeMode()) {
    sendClockStatus(nSessionId);
  }
}

void ClockControl::sendNow(uint32_t nSessionId)
{
  admin::Message message;
  message.mutable_system_clock()->set_now(m_pSystemManager->getClock().now());
  m_pChannel->send(nSessionId, message);
}

void ClockControl::sendStatus(uint32_t nSessionId, admin::SystemClock::Status eStatus)
{
  admin::Message message;
  message.mutable_system_clock()->set_status(eStatus);
  m_pChannel->send(nSessionId, message);
}

void ClockControl::sendClockStatus(uint32_t nSessionId)
{
  utils::Clock& clock = m_pSystemManager->getClock();
  if (clock.isInRealTimeMode()) {
    sendStatus(nSessionId, admin::SystemClock::MODE_REAL_TIME);
  } else if (clock.isInDebugMode()) {
    sendStatus(nSessionId, admin::SystemClock::MODE_DEBUG);
  } else if (clock.isTerminated()) {
    sendStatus(nSessionId, admin::SystemClock::MODE_TERMINATED);
  } else {
    sendStatus(nSessionId, admin::SystemClock::INTERNAL_ERROR);
    assert("Unexpected clock state!" == nullptr);
  }
}

} // namespace administrator

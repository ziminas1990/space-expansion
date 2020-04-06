//#include "SystemClock.h"

//#include <SystemManager.h>

//namespace administrator {

//void SystemClock::proceed()
//{
//  if (m_eState == eIdle) {
//    return;
//  }
//  assert(m_eState == eBusy);

//  if (m_pSystemManager->getClockState() == SystemManager::ClockState::eManualMode) {
//    // System manager is still in manual mode, so continue waiting
//    return;
//  }

//  // System manager switchd from manual mode to freezed
//  assert(m_pSystemManager->getClockState() == SystemManager::ClockState::eFreezed);
//  sendNow();
//}

//void SystemClock::handleMessage(uint32_t nSessionId, admin::SystemClock message)
//{
//  if (m_eState == eBusy) {
//    assert(m_pSystemManager->getClockState() == SystemManager::ClockState::eManualMode);
//    sendClockStatus(nSessionId, admin::SystemClock::CLOCK_IS_BUSY);
//    return;
//  }

//  switch(message.choice_case()) {
//    case admin::SystemClock::kTime:
//      sendNow(nSessionId);
//      return;
//    case admin::SystemClock::kStatusReq:
//      sendTimingStatus(nSessionId);
//      return;
//    case admin::SystemClock::kRun:
//      m_pSystemManager->resume();
//      sendTimingStatus(nSessionId);
//      return;
//    case admin::SystemClock::kFreeze:
//      m_pSystemManager->freeze();
//      sendTimingStatus(nSessionId);
//      return;
//    case admin::SystemClock::kProceed:
//      if (m_pSystemManager->getClockState() != SystemManager::ClockState::eFreezed) {
//        sendClockStatus(nSessionId, admin::SystemClock::CLOCK_IS_NOT_FREEZED);
//        return;
//      }
//      m_pSystemManager->proceedInterval(message.proceed().tick_us(),
//                                        message.proceed().interval_us());
//      sendTimingStatus(nSessionId);
//    default:
//      assert("Unexpected message" == nullptr);
//  }
//}

//void SystemClock::sendNow(uint32_t nSessionId)
//{
//  admin::Message message;
//  message.mutable_system_clock()->set_now(
//        static_cast<uint64_t>(
//          m_pSystemManager->now().count()));
//  m_pChannel->send(nSessionId, message);
//}

//void SystemClock::sendTimingStatus(uint32_t nSessionId)
//{
//  switch(m_pSystemManager->getClockState()) {
//    case SystemManager::ClockState::eRunInRealTime:
//      sendClockStatus(nSessionId, admin::SystemClock::RUNNING);
//      return;
//    case SystemManager::ClockState::eFreezed:
//      sendClockStatus(nSessionId, admin::SystemClock::FREEZED);
//      return;
//    case SystemManager::ClockState::eTerminate:
//      sendClockStatus(nSessionId, admin::SystemClock::TERMINATED);
//      return;
//    default:
//      assert("Unexpected SystemManager state!" == nullptr);
//      return;
//  }
//}

//void SystemClock::sendClockStatus(uint32_t nSessionId,
//                                  admin::SystemClock::Status eStatus)
//{
//  admin::Message message;
//  message.mutable_system_clock()->set_status(eStatus);
//  m_pChannel->send(nSessionId, message);
//}

//} // namespace administrator

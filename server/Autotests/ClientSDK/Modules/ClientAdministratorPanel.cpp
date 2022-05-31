#include "ClientAdministratorPanel.h"

namespace autotests { namespace client {

static SystemClockMode convertClockMode(admin::SystemClock::Status mode)
{
  switch (mode) {
    case admin::SystemClock::MODE_REAL_TIME:
      return SystemClockMode::eRealTime;
    case admin::SystemClock::MODE_DEBUG:
      return SystemClockMode::eDebug;
    case admin::SystemClock::MODE_TERMINATED:
      return SystemClockMode::eTerminated;
    default:
      return SystemClockMode::eUnknown;
  }
}

static AdministratorPanel::Status convertClockStatus(admin::SystemClock::Status status)
{
  switch (status) {
    case admin::SystemClock::CLOCK_IS_BUSY:
      return AdministratorPanel::eClockIsBusy;
    case admin::SystemClock::INTERNAL_ERROR:
      return AdministratorPanel::eClockInternalError;
    default:
      return AdministratorPanel::eStatusError;
  }
}


AdministratorPanel::Status AdministratorPanel::login(
    std::string const& sLogin, std::string const& sPassword, uint64_t& nToken)
{
  admin::Message message;
  admin::Access::Login* pBody = message.mutable_access()->mutable_login();
  pBody->set_login(sLogin);
  pBody->set_password(sPassword);

  if (!m_pPrivilegedPipe->send(std::move(message))) {
    return eTransportError;
  }

  admin::Access response;
  if (!m_pPrivilegedPipe->wait(response)) {
    return eTimeoutError;
  }

  switch (response.choice_case()) {
    case admin::Access::kSuccess:
      nToken = response.success();
      return eSuccess;
    case admin::Access::kFail:
      return eLoginFailed;
    default:
      return eUnexpectedMessage;
  }
}

AdministratorPanel::Status AdministratorPanel::clockTimeReq(uint64_t& nTime)
{
  admin::Message message;
  message.mutable_system_clock()->set_time_req(true);

  if (!m_pPrivilegedPipe->send(std::move(message))) {
    return eTransportError;
  }

  return waitClockTime(nTime);
}

AdministratorPanel::Status AdministratorPanel::clockModeReq(SystemClockMode& mode)
{
  admin::Message message;
  message.mutable_system_clock()->set_mode_req(true);

  if (!m_pPrivilegedPipe->send(std::move(message))) {
    return eTransportError;
  }

  return waitClockMode(mode);
}

AdministratorPanel::Status AdministratorPanel::switchToRealTime()
{
  admin::Message message;
  message.mutable_system_clock()->set_switch_to_real_time(true);

  if (!m_pPrivilegedPipe->send(std::move(message))) {
    return eTransportError;
  }

  SystemClockMode mode;
  Status rc = waitClockMode(mode);
  if (rc != eSuccess) {
    return rc;
  }

  return waitExactClockMode(SystemClockMode::eRealTime);
}

AdministratorPanel::Status AdministratorPanel::switchToDebugMode()
{
  admin::Message message;
  message.mutable_system_clock()->set_switch_to_debug_mode(true);

  if (!m_pPrivilegedPipe->send(std::move(message))) {
    return eTransportError;
  }

  SystemClockMode mode;
  Status rc = waitClockMode(mode);
  if (rc != eSuccess) {
    return rc;
  }

  return waitExactClockMode(SystemClockMode::eDebug);
}

AdministratorPanel::Status AdministratorPanel::terminate()
{
  admin::Message message;
  message.mutable_system_clock()->set_terminate(true);

  if (!m_pPrivilegedPipe->send(std::move(message))) {
    return eTransportError;
  }

  SystemClockMode mode;
  Status rc = waitClockMode(mode);
  if (rc != eSuccess) {
    return rc;
  }

  return waitExactClockMode(SystemClockMode::eTerminated);
}

AdministratorPanel::Status AdministratorPanel::setTickDuration(uint64_t nTickUs)
{
  admin::Message message;
  message.mutable_system_clock()->set_tick_duration_us(nTickUs);

  if (!m_pPrivilegedPipe->send(std::move(message))) {
    return eTransportError;
  }

  return waitExactClockMode(SystemClockMode::eDebug);
}

AdministratorPanel::Status AdministratorPanel::proceed(uint32_t nTicks, uint64_t& nTime)
{
  admin::Message message;
  message.mutable_system_clock()->set_proceed_ticks(nTicks);

  if (!m_pPrivilegedPipe->send(std::move(message))) {
    return eTransportError;
  }

  return waitClockTime(nTime);
}

AdministratorPanel::Status AdministratorPanel::waitClockTime(uint64_t& nTime)
{
  admin::SystemClock response;
  if (!m_pPrivilegedPipe->wait(response)) {
    return eTimeoutError;
  }

  switch (response.choice_case()) {
    case admin::SystemClock::kNow:
      nTime = response.now();
      return eSuccess;
    case admin::SystemClock::kStatus:
      return convertClockStatus(response.status());
    default:
      return eUnexpectedMessage;
  }
}

AdministratorPanel::Status AdministratorPanel::waitClockMode(SystemClockMode& mode)
{
  admin::SystemClock response;
  if (!m_pPrivilegedPipe->wait(response)) {
    return eTimeoutError;
  }
  if (response.choice_case() != admin::SystemClock::kStatus) {
    return eUnexpectedMessage;
  }
  mode = convertClockMode(response.status());
  return eSuccess;
}

AdministratorPanel::Status
AdministratorPanel::waitExactClockMode(SystemClockMode expected)
{
  admin::SystemClock response;
  if (!m_pPrivilegedPipe->wait(response)) {
    return eTimeoutError;
  }
  if (response.choice_case() != admin::SystemClock::kStatus) {
    return eUnexpectedMessage;
  }
  if (expected == convertClockMode(response.status())) {
    return eSuccess;
  }
  return convertClockStatus(response.status());
}

}}  // namespace autotests::client

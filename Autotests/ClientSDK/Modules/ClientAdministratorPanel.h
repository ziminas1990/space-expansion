#pragma once

#include <memory>

#include <Autotests/ClientSDK/SyncPipe.h>

namespace autotests { namespace client {

enum class SystemClockMode {
  eRealTime,
  eDebug,
  eTerminated,
  eUnknown
};

class AdministratorPanel
{
public:
  enum Status {
    eSuccess,
    eLoginFailed,

    // Errors from server side:
    eClockIsBusy,
    eClockInternalError,

    // Errors, detected on client side:
    eTransportError,
    eUnexpectedMessage,
    eTimeoutError,
    eStatusError
  };

  void attachToChannel(PrivilegedPipePtr pSyncPipe) { m_pPrivilegedPipe = pSyncPipe; }
  void detachChannel() { m_pPrivilegedPipe.reset(); }

  Status login(std::string const& sLogin, std::string const& sPassword,
               uint64_t& nToken);

  // Functions from SystemClock interface:
  Status clockTimeReq(uint64_t& nTime);
  Status clockModeReq(SystemClockMode& mode);
  Status switchToRealTime();
  Status switchToDebugMode();
  Status terminate();
  Status setTickDuration(uint64_t nTickUs);
  Status proceed(uint32_t nTicks, uint64_t& nTimeAfterProceed);

private:
  Status waitClockTime(uint64_t& nTime);
  Status waitClockMode(SystemClockMode& mode);
  Status waitExactClockMode(SystemClockMode expected);

private:
  PrivilegedPipePtr m_pPrivilegedPipe;
};

using AdministratorPanelPtr = std::shared_ptr<AdministratorPanel>;

}}  // namespace autotests::client

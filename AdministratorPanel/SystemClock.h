#pragma once

#include <Network/Interfaces.h>
#include <Priveledeged.pb.h>

class SystemManager;

namespace administrator {

class SystemClock
{
  enum State {
    eIdle,
    eBusy,
  };

public:
  void setup(SystemManager* pSystemManager, network::IPrivilegedChannelPtr pChannel) {
    m_pSystemManager = pSystemManager;
    m_pChannel       = pChannel;
  }

  void proceed();
  void handleMessage(uint32_t nSessionId, admin::SystemClock message);

private:
  void sendNow(uint32_t nSessionId);
  void sendTimingStatus(uint32_t nSessionId);
  void sendClockStatus(uint32_t nSessionId, admin::SystemClock::Status eStatus);

private:
  State                          m_eState = eIdle;
  SystemManager*                 m_pSystemManager = nullptr;
  network::IPrivilegedChannelPtr m_pChannel;
};

} // namespace admin

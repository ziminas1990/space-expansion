#pragma once

#include <Network/Interfaces.h>
#include <Priveledeged.pb.h>

namespace utils { class Clock; }
class SystemManager;

namespace administrator {

class ClockControl
{
public:
  void setup(SystemManager* pSystemManager, network::IPrivilegedChannelPtr pChannel) {
    m_pSystemManager = pSystemManager;
    m_pChannel       = pChannel;
  }

  void proceed();
  void handleMessage(uint32_t nSessionId, admin::SystemClock message);

private:
  utils::Clock& clock();

  void onSwtichToRealTimeReq(uint32_t nSessionId);

  void sendNow(uint32_t nSessionId);
  void sendClockStatus(uint32_t nSessionId);

  void sendStatus(uint32_t nSessionId, admin::SystemClock::Status eStatus);

private:
  SystemManager*                 m_pSystemManager = nullptr;
  network::IPrivilegedChannelPtr m_pChannel;

  uint32_t m_nDebugSessionId = network::gInvalidSessionId;
    // Session, that switched clock to debug state.
};

} // namespace admin

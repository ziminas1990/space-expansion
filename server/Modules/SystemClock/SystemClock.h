#pragma once

#include <memory>
#include <vector>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/SubscriptionsBox.h>
#include <Protocol.pb.h>

namespace modules {

class SystemClock : public BaseModule, public utils::GlobalObject<SystemClock>
{
public:
  SystemClock(std::string&& sName, world::PlayerWeakPtr pOwner);

  void proceed(uint32_t nIntervalUs) override;

protected:
  // override from BaseModule
  void handleSystemClockMessage(uint32_t, spex::ISystemClock const&) override;

  void onSessionClosed(uint32_t nSessionId) override;

private:
  void waitFor(uint32_t nSessionId, uint64_t time);
  void waitUntil(uint32_t nSessionId, uint64_t time);
  void monitoring(uint32_t nSessionId, uint32_t nIntervalMs);

  void sendTime(uint32_t nSessionId);
  void sendRing(uint32_t nSessionId, uint64_t time);

  void drawnLastRing();
  void forgetRingSession(uint32_t nSessionId);

private:
  struct Ring {
    Ring() : nSessionId(0), nWhen(0) {}
    Ring(uint32_t nSessionId, uint64_t nWhen)
      : nSessionId(nSessionId), nWhen(nWhen)
    {}

    bool isValid() const { return nWhen > 0; }

    uint32_t nSessionId;
    uint64_t nWhen;
  };

  std::vector<Ring> m_rings;
  // Array of all rings, that should be sent. Array is sorted in
  // desending ring time (first element has the biggest timestamp)

  utils::SubscriptionsBox m_subscriptions;
  // All sessions that requested monitoring
};

using SystemClockPtr = std::shared_ptr<SystemClock>;

} // namespace modules

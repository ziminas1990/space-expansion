#pragma once

#include <memory>
#include <vector>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Protocol.pb.h>

namespace modules {

class SystemClock : public BaseModule, public utils::GlobalContainer<SystemClock>
{
public:
  SystemClock(std::string&& sName, world::PlayerWeakPtr pOwner);

  void proceed(uint32_t nIntervalUs) override;

protected:
  // override from BaseModule
  void handleSystemClockMessage(uint32_t, spex::ISystemClock const&) override;

private:
  void waitFor(uint32_t nSessionId, uint64_t time);
  void waitUntil(uint32_t nSessionId, uint64_t time);

  void sendTime(uint32_t nSessionId);
  void sendRing(uint32_t nSessionId, uint64_t time);

  void drawnLastRing();
private:

  struct Ring {
    Ring(uint32_t nSessionId, uint64_t nWhen)
      : nSessionId(nSessionId), nWhen(nWhen)
    {}

    uint32_t nSessionId;
    uint64_t nWhen;
  };

  // Array of all rings, that should be sent. Array is sorted in
  // desending ring time (first element has the biggest timestamp)
  std::vector<Ring> m_rings;

};

using SystemClockPtr = std::shared_ptr<SystemClock>;

} // namespace modules

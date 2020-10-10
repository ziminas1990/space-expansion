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

  void onSessionClosed(uint32_t nSessionId) override;

private:
  void waitFor(uint32_t nSessionId, uint64_t time);
  void waitUntil(uint32_t nSessionId, uint64_t time);
  void attachGenerator(uint32_t nSessionId);
  void detachGenerator(uint32_t nSessionId);

  void sendTime(uint32_t nSessionId);
  void sendRing(uint32_t nSessionId, uint64_t time);
  void sendGeneratorTick(uint32_t nSessionId);
  void sendGeneratorStatus(uint32_t nSessionId, spex::ISystemClock::Status eStatus);

  void drawnLastRing();
  void forgetGeneratorSession(uint32_t nSessionId);
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

  uint64_t m_nLastCycleTime;
  // When the last generator cycle ended

  std::vector<Ring> m_rings;
  // Array of all rings, that should be sent. Array is sorted in
  // desending ring time (first element has the biggest timestamp)

  std::vector<uint32_t> m_generatorSessions;
  // All sessions, that attached to the generator
};

using SystemClockPtr = std::shared_ptr<SystemClock>;

} // namespace modules

#pragma once

#include <stdint.h>
#include <chrono>

namespace utils {

struct ClockStat {
  uint64_t nTicksCounter = 0;
  uint64_t nRealTimeUs   = 0;
  uint64_t nIngameTimeUs = 0;
  uint64_t nDeviationUs    = 0;
};

// This clock will be used by SystemManager to get proceeding intervals.
// Before each conveyor cycle SystemManager asks clock about intevral, that
// should be passed to conveyor.
// Clock can be switch to the following mods:
// 1. RealTimeMode - clock tries to get intervals, that keep game logic in
//    real time; but slips are possible;
// 2. DebugMode - clock gives intervals according to the instructions of
//    administrator (or unit test).
// In additional, clock stores some statistics
class Clock
{
  enum State {
    eRealTimeMode,
    eDebugMode,
    eTerminated
  };

public:
  void start(bool lDebugMode = false);

  uint32_t now() const { return m_inGameTime; }
    // Return ingame time (not real time!)

  uint32_t getNextInterval();
    // Return interval to proceed game logic
  uint64_t getTicksCounter() const { return m_nTicksCounter; }

  void switchToRealtimeMode()   { m_eState = eRealTimeMode; }
  void switchDebugMode()        { m_eState = eDebugMode; }
  void terminated()             { m_eState = eTerminated; }
  bool isInRealTimeMode() const { return m_eState == eRealTimeMode; }
  bool isInDebugMode()    const { return m_eState == eDebugMode; }
  bool isTerminated()     const { return m_eState == eTerminated; }

  void setDebugTickUs(uint32_t nDurationUs) {
    m_nDebugTickUs = nDurationUs;
  }

  bool proceedRequest(uint32_t nTicks);
    // Set a number of ticks, that should be proceede in debug mode.
    // Return false if clock is NOT in debug mode or prvious proceed request
    // has not been completed yet.

  void exportStat(ClockStat &out) const;

private:
  State m_eState;

  std::chrono::system_clock::time_point m_startedAt;
    // Time when the clock has been started
  uint32_t m_inGameTime;
    // How much time has passed in the game's world since it has been run
  uint32_t m_nDeviationUs;
    // How much the real time differs from the ingame time. Real time can be
    // calculated as: m_startedAt + m_inGameTime + m_DeviationUs

  // This are used in debug mode only
  uint32_t m_nDebugTickUs;
  uint32_t m_nDebugTicksCounter;

  // For statistic purposes only
  uint64_t m_nTicksCounter = 0;
};

} // namespace utils

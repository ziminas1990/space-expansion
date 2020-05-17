#pragma once

#include <stdint.h>
#include <chrono>

namespace utils {

struct ClockStat {
  uint64_t nTicksCounter = 0;
  uint64_t nRealTimeUs   = 0;
  uint64_t nIngameTimeUs = 0;
  int64_t  nDeviationUs   = 0;
  uint64_t nAvgTickDurationPerPeriod = 0;
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
  void start(bool lColdStart = false);

  uint64_t now() const { return m_inGameTime; }
    // Return ingame time (not real time!)

  uint32_t getNextInterval();
    // Return interval to proceed game logic

  bool switchToRealtimeMode();
  bool switchToDebugMode();
  void terminate();
  bool isInRealTimeMode()  const { return m_eState == eRealTimeMode; }
  bool isInDebugMode()     const { return m_eState == eDebugMode; }
  bool isDebugInProgress() const { return isInDebugMode() && m_nDebugTicksCounter; }
  bool isTerminated()      const { return m_eState == eTerminated; }

  void setDebugTickUs(uint32_t nDurationUs) {
    m_nDebugTickUs = nDurationUs;
  }

  bool proceedRequest(uint32_t nTicks);
    // Set a number of ticks, that should be proceede in debug mode.
    // Return false if clock is NOT in debug mode or prvious proceed request
    // has not been completed yet.

  void exportStat(ClockStat &out) const;

private:
  State m_eState = eRealTimeMode;

  std::chrono::system_clock::time_point m_startedAt;
    // Time when the clock has been started
  uint64_t m_inGameTime   = 0;
    // How much time has passed in the game's world since it has been run
  int64_t m_nDeviationUs = 0;
    // How much the real time differs from the ingame time. Real time can be
    // calculated as: m_startedAt + m_inGameTime + m_DeviationUs
    // Negative value means, that ingame time flowed faster then real time
    // (it is possible when system clock is controlled manually)

  // This are used in debug mode only
  uint32_t m_nDebugTickUs       = 0;
  uint32_t m_nDebugTicksCounter = 0;

  // For statistic purposes only
  uint64_t         m_nTotalTicksCounter  = 0;
  mutable uint64_t m_nPeriodTicksCounter = 0;
  mutable uint64_t m_nPeriodDurationUs   = 0;
};

} // namespace utils

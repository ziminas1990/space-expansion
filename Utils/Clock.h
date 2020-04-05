#pragma once

#include <stdint.h>
#include <chrono>

namespace utils {

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
    eDebugMode
  };

public:
  Clock();

  void start(bool lDebugMode = false);

  void switchToRealtimeMode()   { m_eState = eRealTimeMode; }
  void switchDebugMode()        { m_eState = eDebugMode; }
  bool isInRealTimeMode() const { return m_eState == eRealTimeMode; }
  bool isInDebugMode()    const { return m_eState == eDebugMode; }

  uint32_t getNextInterval();

private:
  State m_eState;

  std::chrono::system_clock::time_point m_startedAt;
    // Time when clock has been started
  std::chrono::microseconds    m_inGameTime;
    // How much time has passed in the game's world scince it was run
  std::chrono::microseconds    m_freezedTime;
    // How much time the world has been freezed

  // This are used in debug mode only
  std::chrono::microseconds m_nDebugTickUs;
  uint32_t                  m_nDebugTicksCounter;
};

} // namespace utils

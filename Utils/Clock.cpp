#include "Clock.h"
#include <assert.h>

namespace utils {

inline uint64_t timeSinceUs(std::chrono::system_clock::time_point point)
{
  auto now = std::chrono::high_resolution_clock::now();
  return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - point)
        .count());
}

void Clock::start(bool lDebugMode)
{
  if (lDebugMode) {
    m_eState = eDebugMode;
  }
  m_startedAt    = std::chrono::high_resolution_clock::now();
  m_inGameTime   = 0;
  m_nDeviationUs = 0;
}

uint32_t Clock::getNextInterval()
{
  const uint64_t nMaxTickUs = 10000; // 10 ms

  // expected_now = m_startedAt + m_inGameTime + m_DeviationUs
  // dt = actual_now - expected_now
  uint64_t dt  = timeSinceUs(m_startedAt);
  dt          -= m_inGameTime + m_nDeviationUs;

  switch (m_eState) {
    case eRealTimeMode: {
      if (dt > nMaxTickUs) {
        // Ooops, seems there is a perfomance problem (slip detected)
        m_nDeviationUs += dt - nMaxTickUs;
        dt = nMaxTickUs;
      }
      // return instead of break to avoid extra jump
      assert(dt < uint32_t(-1));
      m_inGameTime        += dt;
      m_nPeriodDurationUs += dt;
      ++m_nTotalTicksCounter;
      ++m_nPeriodTicksCounter;
      return static_cast<uint32_t>(dt);
    }
    case eDebugMode: {
      m_nDeviationUs += dt;
      if (m_nDebugTickUs) {
        dt = m_nDebugTickUs;
        ++m_nTotalTicksCounter;
        ++m_nPeriodTicksCounter;
      } else {
        dt = 0;
      }
      assert(dt < uint32_t(-1));
      m_nDeviationUs -= dt;
      --m_nDebugTicksCounter;
      // return instead of break to avoid extra jump
      m_inGameTime        += dt;
      m_nPeriodDurationUs += dt;
      return static_cast<uint32_t>(dt);
    }
    default: {
      assert("Unexpected state!" == nullptr);
      return 0;
    }
  }
}

bool Clock::switchToRealtimeMode()
{
  switch (m_eState) {
    case eDebugMode:
      if (m_nDebugTicksCounter > 0) {
        return false;
      }
      m_eState = eRealTimeMode;
      return true;
    case eRealTimeMode:
      return true;
    case eTerminated:
      return false;
  }
  return false;
}

bool Clock::switchToDebugMode()
{
  switch (m_eState) {
    case eRealTimeMode:
      m_eState = eDebugMode;
      return true;
    case eDebugMode:
      return true;
    case eTerminated:
      return false;
  }
  return false;
}

bool Clock::proceedRequest(uint32_t nTicks)
{
  if (m_eState != eDebugMode || m_nDebugTicksCounter > 0) {
    return false;
  }
  m_nDebugTicksCounter = nTicks;
  return true;
}

void Clock::exportStat(ClockStat& out) const
{
  out.nTicksCounter = m_nTotalTicksCounter;
  out.nRealTimeUs   = timeSinceUs(m_startedAt);
  out.nIngameTimeUs = m_inGameTime;
  out.nDeviationUs  = m_nDeviationUs;
  out.nAvgTickDurationPerPeriod = m_nPeriodDurationUs / m_nPeriodTicksCounter;

  m_nPeriodTicksCounter = 0;
  m_nPeriodDurationUs   = 0;
}

} // namespace utils

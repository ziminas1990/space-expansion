#include "Clock.h"
#include <assert.h>

#ifdef DEBUG_MODE
#include <iostream>
#endif

namespace utils {

inline uint64_t timeSinceUs(std::chrono::system_clock::time_point point)
{
  auto now = std::chrono::high_resolution_clock::now();
  return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - point)
        .count());
}

uint32_t Clock::getNextInterval()
{
  const uint64_t nMaxTickUs = 20000; // 20 ms

  ++m_nTicksCounter;

  // expected_now = m_startedAt + m_inGameTime + m_DeviationUs
  // dt = actual_now - expected_now
  uint64_t dt  = timeSinceUs(m_startedAt);
  dt          -= m_inGameTime + m_nDeviationUs;

  switch (m_eState) {
    case eRealTimeMode: {
      if (dt > nMaxTickUs) {
        // Ooops, seems there is a perfomance problem (slip detected)
        m_nDeviationUs += dt - nMaxTickUs;
#ifdef DEBUG_MODE
        std::cout << "slip detected: " << (dt - nMaxTickUs) << " usec" << std::endl;
#endif // ifdef DEBUG_MODE
        dt = nMaxTickUs;
      }
      // return instead of break to avoid extra jump
      m_inGameTime += dt;
      assert(dt < uint32_t(-1));
      return static_cast<uint32_t>(dt);
    }
    case eDebugMode: {
      m_nDeviationUs += dt;
      dt = m_nDebugTicksCounter ? m_nDebugTickUs : 0;
      m_nDeviationUs -= dt;
      --m_nDebugTicksCounter;
      // return instead of break to avoid extra jump
      m_inGameTime += dt;
      assert(dt < uint32_t(-1));
      return static_cast<uint32_t>(dt);
    }
    default: {
      assert("Unexpected state!" == nullptr);
      return 0;
    }
  }
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
  out.nTicksCounter = m_nTicksCounter;
  out.nRealTimeUs   = timeSinceUs(m_startedAt);
  out.nIngameTimeUs = m_inGameTime;
  out.nDeviationUs  = m_nDeviationUs;
}

} // namespace utils

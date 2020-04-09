#include "Printers.h"

#include <sstream>

namespace utils {

std::string toTime(uint64_t nIntervalUs)
{
  std::stringstream ss;
  if (nIntervalUs < 50000) {
    // for the small time intervals (less thatn 50 ms)
    ss << nIntervalUs << "us";
    return ss.str();
  }

  uint64_t nIntervalMs = nIntervalUs / 1000;

  uint64_t nHours    = nIntervalMs / (3600 * 1000);
  nIntervalMs       %= 3600 * 1000;
  uint64_t nMinutes  = nIntervalMs / (60 * 1000);
  nIntervalMs       %= 60 * 1000;
  uint64_t nSeconds  = nIntervalMs / 1000;
  nIntervalMs       %= 1000;

  if (nHours) {
    ss << nHours << "h ";
  }
  if (nMinutes) {
    ss << nMinutes << "m ";
  }
  ss << nSeconds << ".";
  ss << nIntervalMs << "s";
  return ss.str();
}

} // namespace utils

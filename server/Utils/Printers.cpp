#include "Printers.h"

#include <sstream>

namespace utils {

std::string toTime(uint64_t nIntervalUs)
{
  std::stringstream ss;
  if (nIntervalUs < 50000) {
    // for the small time intervals (less than 50 ms)
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
  if (nIntervalMs < 100) {
    ss << '0';
    if (nIntervalMs < 10) {
      ss << '0';
    }
  }
  ss << nIntervalMs << "s";
  return ss.str();
}

std::string toTime(int64_t nIntervalUs)
{
  std::stringstream ss;
  if (nIntervalUs < 0) {
    ss << '-';
  }
  ss << toTime(static_cast<uint64_t>(std::abs(nIntervalUs)));
  return ss.str();
}

} // namespace utils

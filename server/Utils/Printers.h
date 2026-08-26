#pragma once

#include <string>
#include <stdint.h>

namespace utils {

std::string toTime(uint64_t nIntervalUs);
std::string toTime(int64_t nIntervalUs);

}   // namespace utils

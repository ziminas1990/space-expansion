#pragma once

#include <chrono>

namespace utils {

class Stopwatch
{
  using clock = std::chrono::high_resolution_clock;

public:
  Stopwatch() { reset(); }

  void reset() {
    m_start = clock().now();
  }

  uint64_t testUs() const {
    return static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::microseconds>(test()).count());
  }

  uint32_t testMs() const {
    return static_cast<uint32_t>(
          std::chrono::duration_cast<std::chrono::milliseconds>(test()).count());
  }

private:
  clock::duration test() const {
    return clock().now() - m_start;
  }

private:
  clock::time_point m_start;

};

} // namespace utils

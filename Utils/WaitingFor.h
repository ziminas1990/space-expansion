#include <functional>
#include <chrono>
#include <thread>

namespace utils {

// This function waits until predicate becomes true and calling proceeder during waiting.
// If predicate becomes true, function also returns true immediatelly.
// After nTimeoutMs ms of waiting function will return false.
// This function will be used only in autotests.
static bool waitFor(std::function<bool()> predicate,
                    std::function<void()> proceeder,
                    uint16_t nTimeoutMs = 100)
{
  std::chrono::milliseconds nTimeout(nTimeoutMs);
  auto start = std::chrono::high_resolution_clock::now();
  std::chrono::milliseconds duration;
  do {
    proceeder();
    if(predicate())
      return true;
    std::this_thread::yield();
    duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now() - start);
  } while(duration < nTimeout);
  // timeout
  return false;
}

} // namespace utils

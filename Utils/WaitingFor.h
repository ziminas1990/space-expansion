#include <functional>
#include <chrono>
#include <thread>

namespace utils {

bool waitFor(std::function<bool()> predicate, std::function<void()> proceeder,
             uint16_t nTimeoutMs = 500)
{
  std::chrono::milliseconds nTimeout(nTimeoutMs);
  auto start = std::chrono::high_resolution_clock::now();
  std::chrono::milliseconds duration;
  do {
    if(predicate())
      return true;
    proceeder();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now() - start);
  } while(duration < nTimeout);
  // timeout
  return false;
}

} // namespace utils

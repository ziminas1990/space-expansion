#pragma once

#ifdef MUTEX_ONLY_MODE
#include <mutex>
#else
#include <atomic>
#endif

namespace utils
{

class Mutex
{
public:

#ifdef MUTEX_ONLY_MODE
  void lock()   { m_mutex.lock(); }
  void unlock() { m_mutex.unlock(); }
#else
  void lock()   { while (m_lFlag.test_and_set(std::memory_order_acquire)); }
  void unlock() { m_lFlag.clear(std::memory_order_release); }
#endif

private:
#ifdef MUTEX_ONLY_MODE
  std::mutex m_mutex;
#else
  std::atomic_flag m_lFlag = ATOMIC_FLAG_INIT ;
#endif
};

} // namespace utils

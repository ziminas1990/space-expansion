#pragma once

#include <atomic>

namespace utils
{

class SpinLock
{
public:
  void lock()   { while (m_lFlag.test_and_set(std::memory_order_acquire)); }
  void unlock() { m_lFlag.clear(std::memory_order_release); }

private:
  std::atomic_flag m_lFlag = ATOMIC_FLAG_INIT ;
};

} // namespace utils

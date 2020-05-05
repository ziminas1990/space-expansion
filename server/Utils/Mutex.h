#pragma once

#ifdef SPINLOCKS_ONLY_MODE
#include "Spinlock.h"
#else
#include <mutex>
#endif

namespace utils {
#ifdef SPINLOCKS_ONLY_MODE
using Mutex = Spinlock;   // *laughter of the evil genius*
#else
using Mutex = std::mutex;
#endif
} // namespace utils

#pragma once

#include <assert.h>
#include <mutex>
#include <stdlib.h>
#include <vector>

#include <Utils/Mutex.h>

namespace utils
{

template<typename IntType, IntType nInvalidValue = IntType(-1)>
class SimpleIdPool
{
public:
  SimpleIdPool(IntType nFirst = IntType(nInvalidValue + 1),
             IntType nLast = IntType(nInvalidValue - 1))
    : m_nFirst(nFirst), m_nLast(nLast), m_nNext(nFirst)
  {}

  IntType end() const { return nInvalidValue; }

  IntType getNext()
  {
    if (!m_avaliable.empty()) {
      const IntType nElement = m_avaliable.back();
      m_avaliable.pop_back();
      return nElement;
    } else if (m_nNext <= m_nLast) {
      return m_nNext++;
    }
    return nInvalidValue;
  }

  void release(IntType element)
  {
    if (element + 1 < m_nNext) {
      m_avaliable.push_back(element);
      drawLastElement();
    } else {
      --m_nNext;
    }
  }

  bool    isValid(IntType id) const { return id != nInvalidValue; }
  IntType getInvalidValue()   const { return nInvalidValue; }

private:
  // Move last element of 'm_available' to it's proper position to keep vector
  // sorted
  // TODO: replace multiple std::swap calls with a single memmove() call
  void drawLastElement() {
    if (m_avaliable.size() >= 2) {
      for (size_t i = m_avaliable.size() - 1; i > 0; --i) {
        assert(m_avaliable[i] != m_avaliable[i-1] && "Duplicated id");
        if (m_avaliable[i - 1] < m_avaliable[i]) {
          std::swap(m_avaliable[i - 1], m_avaliable[i]);
        } else {
          break;
        }
      }
    }
  }

private:
  IntType m_nFirst;
  IntType m_nLast;
  IntType m_nNext;
  std::vector<IntType> m_avaliable;
};


template<typename IntType, IntType nInvalidValue = IntType(-1)>
class ThreadSafeIdPool
{
public:
  ThreadSafeIdPool(IntType nFirst = 0, IntType nLast = IntType(nInvalidValue - 1))
    : m_pool(nFirst, nLast)
  {}

  IntType end() const { return nInvalidValue; }

  IntType getNext()
  {
    std::lock_guard<utils::Mutex> guard(m_mutex);
    return m_pool.getNext();
  }

  void release(IntType element)
  {
    std::lock_guard<utils::Mutex> guard(m_mutex);
    m_pool.release(element);
  }

  bool isValid(IntType id) const { return id != nInvalidValue; }

private:
  utils::Mutex m_mutex;
  SimpleIdPool<IntType, nInvalidValue> m_pool;
};


} // namespace utils

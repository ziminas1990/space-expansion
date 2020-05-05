#pragma once

#include <mutex>
#include <stdlib.h>
#include <set>

#include <Utils/Mutex.h>

namespace utils
{

template<typename IntType, IntType nInvalidValue = IntType(-1)>
class SimplePool
{
public:
  SimplePool(IntType nFirst = IntType(nInvalidValue + 1),
             IntType nLast = IntType(nInvalidValue - 1))
    : m_nFirst(nFirst), m_nLast(nLast), m_nNext(nFirst)
  {}

  IntType end() const { return nInvalidValue; }

  IntType getNext()
  {
    if (!m_Avaliable.empty()) {
      IntType nElement = *m_Avaliable.begin();
      m_Avaliable.erase(m_Avaliable.begin());
      return nElement;
    } else if (m_nNext <= m_nLast) {
      return m_nNext++;
    }
    return nInvalidValue;
  }

  void release(IntType element)
  {
    if (element + 1 == m_nNext) {
      --m_nNext;
      auto I = m_Avaliable.find(m_nNext - 1);
      while (I != m_Avaliable.end()) {
        m_Avaliable.erase(I);
        --m_nNext;
        I = m_Avaliable.find(m_nNext - 1);
      }
    } else {
      m_Avaliable.insert(element);
    }
  }

  bool    isValid(IntType id) const { return id != nInvalidValue; }
  IntType getInvalidValue()   const { return nInvalidValue; }

private:
  IntType m_nFirst;
  IntType m_nLast;
  IntType m_nNext;
  std::set<IntType> m_Avaliable;
};


template<typename IntType, IntType nInvalidValue = IntType(-1)>
class ThreadSafePool
{
public:
  ThreadSafePool(IntType nFirst = 0, IntType nLast = IntType(nInvalidValue - 1))
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
  SimplePool<IntType, nInvalidValue> m_pool;
};


} // namespace utils

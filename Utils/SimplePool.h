#pragma once

#include <stdlib.h>
#include <set>

namespace utils
{

template<typename IntType, IntType nInvalidValue = IntType(-1)>
class SimplePool
{
public:
  SimplePool(IntType nFirst, IntType nLast)
    : m_nFirst(nFirst), m_nLast(nLast), m_nNext(nFirst)
  {}

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
        auto I = m_Avaliable.find(m_nNext - 1);
      }
    } else {
      m_Avaliable.insert(element);
    }
  }

private:
  IntType m_nFirst;
  IntType m_nLast;
  IntType m_nNext;
  std::set<IntType> m_Avaliable;
};

} // namespace utils

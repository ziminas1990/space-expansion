#pragma once

#include <assert.h>
#include <memory.h>
#include <vector>
#include <shared_mutex>
#include "SimplePool.h"

namespace utils
{

// This class is designed to store a number of ids and to iterate through them in
// multithreaded enviroment in almost non-blocking way
template<typename IdType, IdType nInvalidValue = IdType(-1)>
class IdArray
{
public:

  IdArray(size_t nInitialCapacity = 64)
    : m_nCapacity(nInitialCapacity), m_nSize(0),
      m_pIds( nInitialCapacity ? new IdType[nInitialCapacity] : nullptr)
  {}

  ~IdArray() { delete [] m_pIds; }

  bool push(IdType id) {
    size_t nPosition = m_indexesPool.getNext();
    if (nPosition == m_indexesPool.end())
      return false;
    m_mutex.lock_shared();
    if (nPosition >= m_nCapacity) {
      m_mutex.unlock_shared();
      m_mutex.lock();
      reallocate(nPosition + 1);
      m_mutex.unlock();
      m_mutex.lock_shared();
    }
    storeId(nPosition, id);
    m_mutex.unlock_shared();
    return true;
  }

  bool empty() const { return m_nSize == 0; }

  void begin() { m_nNextIndex.store(0); }

  // Could be called from different threads to get some id from array
  // To iterate through array you should call begin() first. Than you can call
  // getNextId() till it returns true.
  // To remove id from array you should call remove() and pass an index to it, that
  // was returned via second parameter of getNextId()
  bool getNextId(IdType& id, size_t& index)
  {
    while (true) {
      index = m_nNextIndex.fetch_add(1);
      if (index >= m_nSize)
        return false;
      id = m_pIds[index];
      if (id == nInvalidValue)
        continue;
      return true;
    }
  }

  void forgetIdAtPosition(size_t index)
  {
    bool lSuccess = false;
    m_mutex.lock_shared();
    if (index < m_nSize) {
      m_pIds[index] = nInvalidValue;
      lSuccess = true;
    }
    m_mutex.unlock_shared();
    if (lSuccess)
      m_indexesPool.release(index);
  }

private:
  void reallocate(size_t nRequiredSize)
  {
    // NOTE: m_mutex must be locked here!
    if (nRequiredSize < m_nCapacity) {
      assert(false);
      return;
    }
    size_t nNewCapacity = 2 * m_nCapacity;
    if (nNewCapacity < nRequiredSize)
      nNewCapacity = nRequiredSize * 2;

    IdType* pNewArray = new IdType[nNewCapacity];
    memcpy(pNewArray, m_pIds, m_nCapacity * sizeof(IdType));
    delete [] m_pIds;
    m_pIds = pNewArray;
  }

  void storeId(size_t nPosition, IdType id)
  {
    // NOTE: m_mutex must be shared_locked here!
    m_pIds[nPosition] = id;
    if (nPosition == m_nSize) {
      m_nSize++;
    } else if (nPosition > m_nSize) {
      for(size_t i = m_nSize; i < nPosition; ++i)
        m_pIds[i] = nInvalidValue;
      m_nSize = nPosition + 1;
    }
  }

private:
  ThreadSafePool<size_t> m_indexesPool;

  // Used to protect m_ids. If resizing of m_ids is not required, this mutex should be
  // locked for read. If resizing is possible, this mutex should be locked for write
  std::shared_mutex   m_mutex;
  // std::vector doesn't fit here well
  size_t  m_nCapacity;
  size_t  m_nSize;
  IdType* m_pIds;

  std::atomic_size_t m_nNextIndex;
};

} // namespace utils

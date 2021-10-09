#pragma once

#include <assert.h>
#include <memory.h>
#include <vector>
#include <shared_mutex>
#include <atomic>
#include <stdint.h>
#include "SimplePool.h"

namespace utils
{

// Class represents an array of ids. It supports:
// 1. multithreading iterating
// 2. removing some elements while iterating
// Restrictions:
// 1. begin() fubction must be called before iterating
// 2. can't push elements while iterating
template<typename IdType, IdType nInvalidValue = IdType(-1)>
class IdArray
{
public:

  IdArray(size_t nInitialCapacity = 64)
  {
    m_ids.reserve(nInitialCapacity);
  }

  void push(IdType id) {
    // Add the specified 'id' to array.
    std::unique_lock rwlock(m_mutex);
    m_ids.push_back(id);
  }

  bool empty() const {
    std::shared_lock rlock(m_mutex);
    return m_ids.empty();
  }

  void begin() {
    std::shared_lock rlock(m_mutex);
    m_nNextIndex.store(m_ids.size() - 1);
  }

  // Could be called from different threads to get some id from array
  // To iterate through array you should call begin() first. Than you can call
  // getNextId() till it returns true.
  // To remove id from array you should call dropIndex() and pass an index to it,
  // that was returned via second parameter of getNextId()
  bool getNextId(IdType& id, size_t& index)
  {
    while(true) {
      index = m_nNextIndex.fetch_add(-1);
      if (index < m_ids.size()) {
        id = m_ids[index];
        if (id == nInvalidValue) {
          std::unique_lock rwlock(m_mutex);
          m_ids[index] = m_ids.back();
          m_ids.pop_back();
          continue;
        }
        return true;
      }
      return false;
    }
  }

  // Remove id on the specified 'index'.
  // Note: can be called while iterating
  void dropIndex(size_t index)
  {
    m_ids[index] = nInvalidValue;
  }

private:
  mutable std::shared_mutex m_mutex;
  std::vector<IdType>       m_ids;
  std::atomic_size_t        m_nNextIndex;
};

} // namespace utils

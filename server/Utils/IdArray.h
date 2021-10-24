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
//
// NOTE: container is NOT fully thread-safe! The only thread-safe workflow:
// 1. call begin() to start iteration (call is not thread safe!)
// 2. call getNextId() from different threads while it returns true
//    (thread safe)
// 3. call dropIndex() while iterating in case you want to mark id as removed
//
// The rest of the calls are NOT thread-safe!
template<typename IdType, IdType nInvalidValue = IdType(-1)>
class IdArray
{
public:

  IdArray(size_t nInitialCapacity = 64)
  {
    m_ids.reserve(nInitialCapacity);
    m_nTotalIds.store(0);
  }

  void push(IdType id) {
    // Add the specified 'id' to array.
    if (id != nInvalidValue) {
      m_ids.push_back(id);
      m_nTotalIds.fetch_add(1);
    }
  }

  size_t size() const {
    return m_nTotalIds.load();
  }

  bool empty() const {
    return m_nTotalIds.load() == 0;
  }

  void clearInvalidIds() {
    // Remove all invalid ids from array and return a total number of removed
    // elements
    for (size_t i = 0; i < m_ids.size();) {
      if (m_ids[i] == nInvalidValue) {
        m_ids[i] = m_ids.back();
        m_ids.pop_back();
      } else {
        ++i;
      }
    }
  }

  // Must be called before iterating. Will remove all elements, that
  // were marked as removed during previous iteration.
  void begin() {
    clearInvalidIds();
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
      assert(m_ids[index] != nInvalidValue);
      if (index < m_ids.size()) {
        if (m_ids[index] != nInvalidValue) {
          id = m_ids[index];
          return true;
        }
      } else {
        return false;
      }
    }
  }

  // Remove id on the specified 'index'.
  // Note: can be called while iterating
  void dropIndex(size_t index)
  {
    m_ids[index] = nInvalidValue;
    m_nTotalIds.fetch_add(-1);
  }

private:
  std::vector<IdType>       m_ids;
  std::atomic_size_t        m_nTotalIds;
  std::atomic_size_t        m_nNextIndex;
};

} // namespace utils

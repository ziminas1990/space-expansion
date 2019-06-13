#pragma once

#include <vector>
#include <assert.h>

#include "SimplePool.h"
#include "Mutex.h"

// Inhteriter should contain its namespaces! For ex: "newton::PhysicalObject"
// AvoidDummyWarningHack - hack to avoid dummy warning about extra ";"
#define DECLARE_GLOBAL_CONTAINER_CPP(Inheriter) \
  namespace utils { \
  template<> \
  ThreadSafePool<uint32_t> GlobalContainer<Inheriter>::m_IdPool = ThreadSafePool<uint32_t>(); \
  template<> \
  Mutex GlobalContainer<Inheriter>::m_AllInstancesMutex = Mutex(); \
  template<> \
  std::vector<Inheriter*> GlobalContainer<Inheriter>::m_AllInstances = std::vector<Inheriter*>(); \
  } \
  class AvoidDummyWarningHack


namespace utils {

// NOTE: Inheriting this class you MUST:
// 1. put DECLARE_GLOBAL_CONTAINER_CPP somewhere in your cpp-file with Inheriter
//    name (with all namespaces!)
// 2. call GlobalContainer<Inheriter>::registerSelf(this) in your constructor
template<typename Inheriter>
class GlobalContainer
{
public:
  GlobalContainer() : m_nInstanceId(m_IdPool.getNext())
  {
    // valid pointer would be written when inheriter calls registerSelf(this)
    registerSelf(nullptr);
  }

  virtual ~GlobalContainer()
  {
    std::lock_guard<Mutex> guard(m_AllInstancesMutex);
    assert(m_nInstanceId < m_AllInstances.size());
    m_AllInstances[m_nInstanceId] = nullptr;
    m_IdPool.release(m_nInstanceId);
  }

  uint32_t getInstanceId() const { return m_nInstanceId; }

  static std::vector<Inheriter*> const& getAllInstancies() { return m_AllInstances; }
  static uint32_t   TotalInstancies() { return m_AllInstances.size(); }
  static Inheriter* Instance(uint32_t nInstanceId) {
    assert(nInstanceId < m_AllInstances.size());
    return m_AllInstances[nInstanceId];
  }
  static bool empty() { return m_AllInstances.empty(); }

protected:
  void registerSelf(Inheriter* pSelf)
  {
    std::lock_guard<Mutex> guard(m_AllInstancesMutex);
    assert(m_nInstanceId <= m_AllInstances.size());
    if (m_nInstanceId == m_AllInstances.size()) {
      if (!m_AllInstances.capacity())
        m_AllInstances.reserve(0xFF);
      m_AllInstances.push_back(pSelf);
    } else {
      m_AllInstances[m_nInstanceId] = pSelf;
    }
  }

private:
  uint32_t m_nInstanceId;

  static ThreadSafePool<uint32_t> m_IdPool;
  static Mutex                    m_AllInstancesMutex;
  static std::vector<Inheriter*>  m_AllInstances;
};

} // namespace utils

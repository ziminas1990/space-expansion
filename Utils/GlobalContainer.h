#pragma once

#include <memory>
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
  template<> \
  std::vector<GlobalContainer<Inheriter>::IObserver*>\
  GlobalContainer<Inheriter>::m_observers = \
    std::vector<GlobalContainer<Inheriter>::IObserver*>(); \
  } \
  class AvoidDummyWarningHack


namespace utils {

template<typename ObjectsType>
class IContainerObserver;

// NOTE: Inheriting this class you MUST:
// 1. put DECLARE_GLOBAL_CONTAINER_CPP somewhere in your cpp-file with Inheriter
//    name (with all namespaces!)
// 2. call GlobalContainer<Inheriter>::registerSelf(this) in your constructor
template<typename Inheriter>
class GlobalContainer
{
public:
  using IObserver = IContainerObserver<Inheriter>;

  GlobalContainer() : m_nInstanceId(m_IdPool.getNext())
  {
    // valid pointer would be written when inheriter calls registerSelf(this)
    registerSelf(nullptr);
  }

  virtual ~GlobalContainer()
  {
    std::lock_guard<Mutex> guard(m_AllInstancesMutex);
    assert(m_nInstanceId < m_AllInstances.size());
    if (m_nInstanceId < m_AllInstances.size()) {
      m_AllInstances[m_nInstanceId] = nullptr;
      m_IdPool.release(m_nInstanceId);
      for (IObserver* pObserver: m_observers) {
        pObserver->onRemoved(m_nInstanceId);
      }
    }
  }

  uint32_t getInstanceId() const { return m_nInstanceId; }

  // WARNING: all static functions are not thread safe. You may call them from
  // several threads because they preform only read operation. But modifing container
  // (create new objects or delete existing) while reading may lead to race condition.

  // Return all objects in caontainers as an array. Note that array may
  // contain null pointers (due to perfomance reason).
  static std::vector<Inheriter*> const& getAllInstancies() { return m_AllInstances; }
  static uint32_t   TotalInstancies() { return m_AllInstances.size(); }
  static Inheriter* Instance(uint32_t nInstanceId) {
    assert(nInstanceId < m_AllInstances.size());
    return m_AllInstances[nInstanceId];
  }
  static bool empty() { return m_AllInstances.empty(); }

  static void addObserver(IObserver* pObserver) {
    // I assume, that application should not register a lot of
    // observers, because it may lead to slow down
    assert(m_observers.size() < 3);
    m_observers.push_back(pObserver);
  }

  static void removeObserver(IObserver* pObserver) {
    size_t nTotalObservers = m_observers.size();
    size_t i = 0;
    for (; i <= nTotalObservers; ++i) {
      if (m_observers[i] == pObserver)
        break;
    }
    if (i == m_observers.size()) {
      assert("Attempt to remove not registered observer" == nullptr);
      return;
    }
    m_observers[i] = m_observers.back();
    m_observers.pop_back();
  }

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
    for (IObserver* pObserver: m_observers) {
      pObserver->onRegistered(m_nInstanceId, pSelf);
    }
  }

private:
  uint32_t m_nInstanceId;

    // Revision increases every time when container is modified
  static ThreadSafePool<uint32_t> m_IdPool;
    // ObjectsId, that can be reused to new objects
  static Mutex                    m_AllInstancesMutex;
  static std::vector<Inheriter*>  m_AllInstances;
  static std::vector<IObserver*>  m_observers;
};


// This interface can be inherited to observe which objects were added or
// removed to the GlobalContainer<ObjectsType>
template<typename ObjectsType>
class IContainerObserver
{
public:
  IContainerObserver() { GlobalContainer<ObjectsType>::addObserver(this); }
  virtual ~IContainerObserver() { GlobalContainer<ObjectsType>::removeObserver(this); }

  virtual void onRegistered(size_t nObjectId, ObjectsType* pObject) = 0;
  virtual void onRemoved(size_t nObjectId) = 0;
};


// This class return all instances of obects with type 'ObjectType' as array
// of 'ObjectType' pointers
// For example, 'BaseObjectsContainer<Asteroid>' container returns all asteroids
// as array of 'Asteroid*'
template<typename ObjectType>
class ObjectsContainer
{
public:
  virtual ~ObjectsContainer() = default;

  // Return all objects as array. Note that array may contan null pointers!
  virtual std::vector<ObjectType*> const& getObjects() {
    return GlobalContainer<ObjectType>::getAllInstancies();
  }
};

template<typename ObjectType>
using ObjectsContainerPtr = std::shared_ptr<ObjectsContainer<ObjectType>>;


// This container returns all objects of type 'ConcreteObjectType' as an array of
// pointers to 'BaseObjectType'. The 'ConcreteObjectType' must be a subclass of
// 'BaseObjectType'.
// For example:
// The ConcreteObjectsContainer<Asteroid, PhysicalObjects> object returns
// all asteroids (objects of type 'Asteroid') as array of pointers to 'PhysicalObject'.
//
// NOTE: if you want to get all asteroids as array of 'Asteroid*', use
// 'ObjectsContainer<Asteroid>' instead due to perfomance reason.
//
// This class is NOT thread safe. It's implementation relies on GlobalContainer's
// synchronization.
template<typename ConcreteObjectType, typename BaseObjectType>
class ConcreteObjectsContainer :
    public ObjectsContainer<BaseObjectType>,
    public IContainerObserver<ConcreteObjectType>
{
public:

  void onRegistered(size_t nObjectId, ConcreteObjectType* pObject) override
  {
    if (nObjectId == m_baseObjects.size()) {
      m_baseObjects.push_back(static_cast<BaseObjectType*>(pObject));
    } else {
      m_baseObjects[nObjectId] = static_cast<BaseObjectType*>(pObject);
    }
  }

  void onRemoved(size_t nObjectId) override
  {
    m_baseObjects[nObjectId] = nullptr;
  }

  // Note that array may contain nullptr's
  std::vector<BaseObjectType*> const& getObjects() override {
    return m_baseObjects;
  }

private:
  std::vector<BaseObjectType*> m_baseObjects;
};

} // namespace utils

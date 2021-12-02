#pragma once

#include <memory>
#include <vector>
#include <assert.h>

#include "SimplePool.h"
#include "Mutex.h"
#include <World/ObjectTypes.h>

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

template<typename ObjectsType>
class GlobalObject;


template<typename Inheriter>
class GlobalContainer
{
  friend class GlobalObject<Inheriter>;
public:
  using IObserver = IContainerObserver<Inheriter>;

  static uint32_t getNextId()            { return m_IdPool.getNext(); }
  static void     releaseId(uint32_t id) { m_IdPool.release(id); }

  // WARNING: all static functions are not thread safe. You may call them from
  // several threads because they preform only read operation. But modifing container
  // (create new objects or delete existing) while reading may lead to race condition.

  // Return all objects in caontainers as an array. Note that array may
  // contain null pointers (due to perfomance reason).
  static std::vector<Inheriter*> const& getAllInstancies() { return m_AllInstances; }
  static uint32_t   TotalInstancies() { return static_cast<uint32_t>(m_AllInstances.size()); }
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

private:
  static ThreadSafePool<uint32_t> m_IdPool;
    // ObjectIds, that can be reused to new objects
  static Mutex                    m_AllInstancesMutex;
  static std::vector<Inheriter*>  m_AllInstances;
  static std::vector<IObserver*>  m_observers;
};


// NOTE: Inheriting this class you MUST:
// 1. put DECLARE_GLOBAL_CONTAINER_CPP somewhere in your cpp-file with Inheriter
//    name (with all namespaces!)
// 2. call GlobalContainer<Inheriter>::registerSelf(this) in your constructor
//
// Optionally you may also:
// 1. override virtual "getType()"
template<typename Inheriter>
class GlobalObject
{
  using Container = GlobalContainer<Inheriter>;

public:
  using IObserver = IContainerObserver<Inheriter>;

  GlobalObject() : m_nInstanceId(Container::getNextId())
  {
    // valid pointer should be written when inheriter calls registerSelf(this)
    registerSelf(nullptr);
  }

  virtual ~GlobalObject()
  {
    std::lock_guard<Mutex> guard(Container::m_AllInstancesMutex);
    assert(m_nInstanceId < Container::m_AllInstances.size());
    if (m_nInstanceId < Container::m_AllInstances.size()) {
      Container::m_AllInstances[m_nInstanceId] = nullptr;
      Container::releaseId(m_nInstanceId);
      for (IObserver* pObserver: Container::m_observers) {
        pObserver->onRemoved(m_nInstanceId);
      }
    }
  }

  virtual world::ObjectType getType()       const {
    return world::ObjectType::eUnspecified;
  }

  uint32_t getInstanceId() const { return m_nInstanceId; }

protected:
  void registerSelf(Inheriter* pSelf)
  {
    std::lock_guard<Mutex> guard(Container::m_AllInstancesMutex);
    assert(m_nInstanceId <= Container::m_AllInstances.size());
    if (m_nInstanceId == Container::m_AllInstances.size()) {
      if (!Container::m_AllInstances.capacity())
        Container::m_AllInstances.reserve(0xFF);
      Container::m_AllInstances.push_back(pSelf);
    } else {
      Container::m_AllInstances[m_nInstanceId] = pSelf;
    }
    for (IObserver* pObserver: Container::m_observers) {
      pObserver->onRegistered(m_nInstanceId, pSelf);
    }
  }

private:
  uint32_t m_nInstanceId;
};


// This interface can be inherited to observe which objects were added or
// removed to the GlobalContainer<ObjectsType>
template<typename ObjectsType>
class IContainerObserver
{
public:
  IContainerObserver() { GlobalContainer<ObjectsType>::addObserver(this); }
  virtual ~IContainerObserver() {
    GlobalContainer<ObjectsType>::removeObserver(this);
  }

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
  virtual std::vector<ObjectType*> const& getObjects() const {
    return GlobalContainer<ObjectType>::getAllInstancies();
  }
};

template<typename ObjectType>
using ObjectsContainerPtr = std::shared_ptr<ObjectsContainer<ObjectType>>;


// This container returns all objects of type 'ConcreteObjectType' as an array of
// pointers to 'BaseObjectType' in O(1) time. The 'ConcreteObjectType' must be a subclass
// of 'BaseObjectType'.
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

  ConcreteObjectsContainer()
  {
    std::vector<ConcreteObjectType*> const& allObjects =
        GlobalContainer<ConcreteObjectType>::getAllInstancies();

    m_baseObjects.reserve(allObjects.size() * 1.1);
    m_baseObjects.insert(m_baseObjects.end(), allObjects.begin(), allObjects.end());
  }

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
  std::vector<BaseObjectType*> const& getObjects() const override {
    return m_baseObjects;
  }

private:
  std::vector<BaseObjectType*> m_baseObjects;
};

} // namespace utils

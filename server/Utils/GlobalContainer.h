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
  ThreadSafePool<uint32_t> GlobalContainer<Inheriter>::gIdPool = ThreadSafePool<uint32_t>(); \
  template<> \
  Mutex GlobalContainer<Inheriter>::gMutex = Mutex(); \
  template<> \
  size_t GlobalContainer<Inheriter>::gRegisteredObjectsCounter = 0; \
  template<> \
  std::vector<Inheriter*> GlobalContainer<Inheriter>::gInstances = std::vector<Inheriter*>(); \
  template<> \
  std::vector<GlobalContainer<Inheriter>::IObserver*>\
  GlobalContainer<Inheriter>::gObservers = \
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

  // WARNING: all static functions are not thread safe. You may call them from
  // several threads because they preform only read operation. But modifing
  // container (create new objects or delete existing) while reading may lead
  // to race condition.

  static std::vector<Inheriter*> const& AllInstancies() { return gInstances; }
  // Return all objects in caontainers as an array. Note that array may
  // contain null pointers (due to perfomance reason).

  static uint32_t   Total() { return static_cast<uint32_t>(gInstances.size()); }

  static Inheriter* Instance(uint32_t nInstanceId) {
    assert(nInstanceId < gInstances.size());
    return gInstances[nInstanceId];
  }

  static bool Empty() { return gRegisteredObjectsCounter == 0; }

  static void AttachObserver(IObserver* pObserver) {
    // I assume that application should not register a lot of
    // gObservers because it may slows it down
    assert(gObservers.size() < 3);
    gObservers.push_back(pObserver);
  }

  static void DetachObserver(IObserver* pObserver) {
    size_t nTotalgObservers = gObservers.size();
    size_t i = 0;
    for (; i <= nTotalgObservers; ++i) {
      if (gObservers[i] == pObserver) {
        gObservers[i] = gObservers.back();
        gObservers.pop_back();
        return;
      }
    }
    if (i == gObservers.size()) {
      assert(!"Attempt to detach not registered observer");
      return;
    }
  }

private:
  static ThreadSafePool<uint32_t> gIdPool;
    // ObjectIds, that can be reused to new objects
  static Mutex                    gMutex;
  static size_t                   gRegisteredObjectsCounter;
  static std::vector<Inheriter*>  gInstances;
  static std::vector<IObserver*>  gObservers;
};


// NOTE: Inheriting this class you MUST:
// 1. put DECLARE_GLOBAL_CONTAINER_CPP somewhere in your cpp-file with Inheriter
//    name (with all namespaces!)
// 2. call GlobalObject<Inheriter>::registerSelf(this) in your constructor
template<typename Inheriter>
class GlobalObject
{
  using Container = GlobalContainer<Inheriter>;

public:
  using IObserver = IContainerObserver<Inheriter>;

  GlobalObject() : m_nInstanceId(Container::gIdPool.getNext())
  {
    // valid pointer should be written when inheriter calls registerSelf(this)
    registerSelf(nullptr);
  }

  GlobalObject(const GlobalObject& other) = delete;
  GlobalObject(GlobalObject&& other) = delete;

  virtual ~GlobalObject()
  {
    std::lock_guard<Mutex> guard(Container::gMutex);
    assert(m_nInstanceId < Container::gInstances.size());
    if (m_nInstanceId < Container::gInstances.size()) {
      Container::gInstances[m_nInstanceId] = nullptr;
      assert(Container::gRegisteredObjectsCounter > 0);
      --Container::gRegisteredObjectsCounter;
      Container::gIdPool.release(m_nInstanceId);
      for (IObserver* pObserver: Container::gObservers) {
        pObserver->onRemoved(m_nInstanceId);
      }
    }
  }

  uint32_t getInstanceId() const { return m_nInstanceId; }

protected:
  void registerSelf(Inheriter* pSelf)
  {
    std::lock_guard<Mutex> guard(Container::gMutex);
    assert(m_nInstanceId <= Container::gInstances.size());
    if (m_nInstanceId == Container::gInstances.size()) {
      if (!Container::gInstances.capacity())
        Container::gInstances.reserve(0xFF);
      Container::gInstances.push_back(pSelf);
    } else {
      Container::gInstances[m_nInstanceId] = pSelf;
    }
    if (pSelf) {
      ++Container::gRegisteredObjectsCounter;
    }
    for (IObserver* pObserver: Container::gObservers) {
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
  IContainerObserver() { GlobalContainer<ObjectsType>::AttachObserver(this); }
  virtual ~IContainerObserver() {
    GlobalContainer<ObjectsType>::DetachObserver(this);
  }

  virtual void onRegistered(size_t nObjectId, ObjectsType* pObject) = 0;
  virtual void onRemoved(size_t nObjectId) = 0;
};


// This class return all gInstances of obects with type 'ObjectType' as array
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
    return GlobalContainer<ObjectType>::AllInstancies();
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
        GlobalContainer<ConcreteObjectType>::AllInstancies();

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

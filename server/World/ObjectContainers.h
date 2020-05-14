#pragma once

#include <memory>
#include <array>
#include <mutex>
#include "ObjectTypes.h"
#include <Utils/GlobalContainer.h>

#include <Newton/PhysicalObject.h>

namespace world {

using PhysicalObjectsContainer = utils::ObjectsContainer<newton::PhysicalObject>;
using PhysicalObjectsContainerPtr = std::shared_ptr<PhysicalObjectsContainer>;
using PhysicalObjectsContainerWeakPtr = std::weak_ptr<PhysicalObjectsContainer>;


// This class provide easy access to containers with different kind of physical
// objects. You can get container with objects of the specified eType and you
// will get a container in O(1) time
//
// Why containers are stored as weak_ptr? Because some containers (that are inherited
// from the 'ConcreteObjectsContainer' class) are not zero-cost and in additional to
// extra memory usage, they slow down operations of creating and removing corresponding
// objects. So, if some container has bee requested and is not used anymore, such
// container should be removed. That's why the 'Containers' class stores containers
// as weak_ptr.
//
// Why mutex? Because class uses weak_ptr create containers "on demand". Such operations
// are not atomic (not just return container) and must be protected with mutex.
class Containers
{
public:
  static PhysicalObjectsContainerPtr getContainerWith(ObjectType eObjectType);

private:
  static std::mutex m_mutex;
    // Mutex to access containers. Mb splitted into several mutex (one per each
    // container)

  static PhysicalObjectsContainerWeakPtr m_pPhysicalObjects;
  static PhysicalObjectsContainerWeakPtr m_pAsteroids;
  static PhysicalObjectsContainerWeakPtr m_pShips;
};


// Can be used as a proxy between client and 'Containers' class.
// It caches containers, that has been created during
// 'Containers::getContainerWith()' call. If container has not been demanded
// for a some peroid of time, it will be dropped from cache
//
// Note: don't forget to call 'proceed()' sometimes, otherwise cache will
// never relese cached containers.
class ContainersCache
{
public:
  ContainersCache() = default;

  PhysicalObjectsContainerPtr getContainerWith(ObjectType eObjectType);

  void proceed(uint32_t nTimeUs);

private:
  struct CachedContainer
  {
    CachedContainer() = default;
    uint32_t                    nTimeToLive = 5000000;
    PhysicalObjectsContainerPtr pContainer  = nullptr;
  };

  std::array<CachedContainer,
             static_cast<size_t>(world::ObjectType::eTotalObjectsTypes)>
  m_cachedContainers;
    // Containers, that has been requested recently
    // Index is ObjectType
};

} // namespace world


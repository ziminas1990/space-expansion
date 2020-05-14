#include "ObjectContainers.h"

#include <World/CelestialBodies/Asteroid.h>
#include <Ships/Ship.h>

namespace world {

PhysicalObjectsContainerWeakPtr Containers::m_pPhysicalObjects;
PhysicalObjectsContainerWeakPtr Containers::m_pAsteroids;
PhysicalObjectsContainerWeakPtr Containers::m_pShips;
std::mutex                      Containers::m_mutex;

PhysicalObjectsContainerPtr Containers::getContainerWith(ObjectType eObjectType)
{
  std::lock_guard<std::mutex> guard(m_mutex);

  switch (eObjectType) {
    case ObjectType::ePhysicalObject: {
      PhysicalObjectsContainerPtr pContainer = m_pPhysicalObjects.lock();
      if (!pContainer) {
        pContainer = std::make_shared<PhysicalObjectsContainer>();
        m_pPhysicalObjects = pContainer;
      }
      return pContainer;
    }
    case ObjectType::eAsteroid: {
      PhysicalObjectsContainerPtr pContainer = m_pAsteroids.lock();
      if (!pContainer) {
        pContainer =
            std::make_shared<utils::ConcreteObjectsContainer<
            world::Asteroid, newton::PhysicalObject>>();
        m_pAsteroids = pContainer;
      }
      return pContainer;
    }
    case ObjectType::eShip: {
      PhysicalObjectsContainerPtr pContainer = m_pShips.lock();
      if (!pContainer) {
        pContainer =
            std::make_shared<utils::ConcreteObjectsContainer<
            ships::Ship, newton::PhysicalObject>>();
        m_pShips = pContainer;
      }
      return pContainer;
    }
    default:
      assert("Access to non existing container" == nullptr);
      return nullptr;
  }
}

//========================================================================================
// ContainersCache::ContainersCache
//========================================================================================

PhysicalObjectsContainerPtr ContainersCache::getContainerWith(ObjectType eObjectType)
{
  assert(eObjectType < world::ObjectType::eTotalObjectsTypes);
  if (eObjectType >= world::ObjectType::eTotalObjectsTypes) {
    return nullptr;
  }

  CachedContainer& cachedItem = m_cachedContainers[static_cast<size_t>(eObjectType)];
  if (!cachedItem.pContainer) {
    cachedItem.pContainer = Containers::getContainerWith(eObjectType);
  }
  cachedItem.nTimeToLive = 5000000; // 5 seconds
  return cachedItem.pContainer;
}

void ContainersCache::proceed(uint32_t nTimeUs)
{
  for (CachedContainer& item : m_cachedContainers) {
    if (!item.pContainer) {
      continue;
    }
    if (item.nTimeToLive < nTimeUs) {
      item.pContainer = nullptr;
      item.nTimeToLive = 0;
    } else {
      item.nTimeToLive -= nTimeUs;
    }
  }
}

} // namespace world

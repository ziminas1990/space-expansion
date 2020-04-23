#include "ObjectContainers.h"

#include <World/CelestialBodies/Asteroid.h>
#include <Ships/Ship.h>

namespace world {

PhysicalObjectsContainerPtr Containers::m_pPhysicalObjects;
PhysicalObjectsContainerPtr Containers::m_pAsteroids;
PhysicalObjectsContainerPtr Containers::m_pShips;

PhysicalObjectsContainerPtr Containers::getContainerWith(ObjectType eObjectType)
{
  switch (eObjectType) {
    case ObjectType::ePhysicalObject:
      if (!m_pPhysicalObjects) {
        m_pPhysicalObjects = std::make_shared<PhysicalObjectsContainer>();
      }
      return m_pPhysicalObjects;
    case ObjectType::eAsteroid:
      if (!m_pAsteroids) {
        m_pAsteroids =
            std::make_shared<utils::ConcreteObjectsContainer<
            world::Asteroid, newton::PhysicalObject>>();
      }
      return m_pAsteroids;
    case ObjectType::eShip:
      if (!m_pShips) {
        m_pShips =
            std::make_shared<utils::ConcreteObjectsContainer<
            ships::Ship, newton::PhysicalObject>>();
      }
      return m_pShips;
    default:
      assert("Access to non existing container" == nullptr);
      return nullptr;
  }
}

} // namespace world

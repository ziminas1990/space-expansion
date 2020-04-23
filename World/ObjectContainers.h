#pragma once

#include <memory>
#include "ObjectTypes.h"
#include <Utils/GlobalContainer.h>

#include <Newton/PhysicalObject.h>

namespace world {

using PhysicalObjectsContainer = utils::ObjectsContainer<newton::PhysicalObject>;
using PhysicalObjectsContainerPtr = std::shared_ptr<PhysicalObjectsContainer>;


// This class provide easy access to containers with different kind of physical
// objects. You can get container with objects of the specified eType and you
// will get a container in O(1) time
class Containers
{
public:
  static PhysicalObjectsContainerPtr getContainerWith(ObjectType eObjectType);

private:
  static PhysicalObjectsContainerPtr m_pPhysicalObjects;
  static PhysicalObjectsContainerPtr m_pAsteroids;
  static PhysicalObjectsContainerPtr m_pShips;
};

} // namespace world

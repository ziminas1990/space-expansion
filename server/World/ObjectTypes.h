#pragma once

namespace world {

enum class ObjectType {
  eUnknown = 0,
  eUnspecified,
  ePhysicalObject,
  eAsteroid,
  eShip,

  // Service fields
  eTotalObjectsTypes
};

} // namespace world

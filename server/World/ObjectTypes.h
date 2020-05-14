#pragma once

namespace world {

enum class ObjectType {
  eUnknown = 0,
  ePhysicalObject,
  eAsteroid,
  eShip,

  // Service fields
  eTotalObjectsTypes
};


} // namespace world

#pragma once

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include "ClientCommutator.h"

namespace autotests { namespace client {

// Ship is a commutator + INavigation interface
class Ship : public ClientCommutator
{
public:
  bool getPosition(geometry::Point& position, geometry::Vector& velocity);
  bool getPosition(geometry::Point& position);
};

using ShipPtr = std::shared_ptr<Ship>;

}} // namespace autotests

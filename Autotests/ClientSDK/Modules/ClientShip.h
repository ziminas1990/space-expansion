#pragma once

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include "ClientCommutator.h"

namespace autotests { namespace client {

// Ship is a commutator + INavigation interface
class ClientShip : public ClientCommutator
{
public:
  bool getPosition(geometry::Point& position, geometry::Vector& velocity);
};

using ClientShipPtr = std::shared_ptr<ClientShip>;

}} // namespace autotests

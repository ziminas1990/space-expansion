#pragma once

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include "ClientCommutator.h"

namespace autotests { namespace client {

struct ShipState {
  double nWeight = 0.001; // to avoid devizion by zero
};

// Ship is a commutator + INavigation interface
class Ship : public ClientCommutator
{
public:
  bool getPosition(geometry::Point& position, geometry::Vector& velocity);
  bool getPosition(geometry::Point& position);

  bool getState(ShipState& state);
};

using ShipPtr = std::shared_ptr<Ship>;

}} // namespace autotests

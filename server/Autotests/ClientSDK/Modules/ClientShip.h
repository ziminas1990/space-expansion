#pragma once

#include <functional>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include "ClientCommutator.h"

namespace autotests { namespace client {

struct ShipState {
  double           nWeight = 0.001; // to avoid devizion by zero
  geometry::Point  position;
  geometry::Vector velocity;
};

// Ship is a commutator + INavigation interface
class Ship : public ClientCommutator
{
public:
  bool getPosition(geometry::Point& position, geometry::Vector& velocity);
  bool getPosition(geometry::Point& position);

  bool getState(ShipState& state);
  bool monitor(uint32_t nPeriodMs, ShipState &state);

  bool waitState(ShipState& state, uint16_t nTimeout = 500);
};

using ShipPtr = std::shared_ptr<Ship>;

}} // namespace autotests

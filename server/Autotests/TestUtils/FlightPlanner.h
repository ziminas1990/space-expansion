#pragma once

#include <stdint.h>
#include <variant>
#include <vector>

#include "Geometry/Vector.h"
#include "Geometry/Point.h"

namespace autotests {

struct HoverEngineBurn {
  uint64_t         nThrust;
  uint32_t         nDurationMs;
  geometry::Vector expectedDirection;
};

struct Turn {
  geometry::Vector direction;
  double           nSpeedRadPerSecond;
};

using Maneuver = std::variant<HoverEngineBurn, Turn>;

struct FlightPlan {
  void addTurn(geometry::Vector direction, double nSpeedRadPerSecond) {
    maneuvers.emplace_back(Turn{direction, nSpeedRadPerSecond});
  }

  void addHoverEngineBurn(
    uint64_t nThrust, uint32_t nDurationMs, geometry::Vector expectedDirection)
  {
    maneuvers.emplace_back(
      HoverEngineBurn{nThrust, nDurationMs, expectedDirection});
  }

  std::vector<Maneuver> maneuvers;
};

struct ShipInfo {
  double nWeight;
  double nMaxThrust;
  double nMaxTurnSpeed;
};

class FlightPlanner
{
public:
  static FlightPlan plan(
    const ShipInfo& shipInfo, geometry::Point start, geometry::Point end);

  static FlightPlan stop(
    const ShipInfo& shipInfo, const geometry::Vector& velocity,
    double minimalBurnDurationMs = 0);
};

} // namespace autotests

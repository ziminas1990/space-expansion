#include "FlightPlanner.h"
#include <cmath>

namespace autotests {

// A simple planner, that assumes that:
// - ship has no velocity and placed at the start point
// - target has no velocity and placed at the end point
// It should be enough to run just four maneuvers:
// - turn to the target direction
// - burn to accelerate towards the target
// - turn back
// - burn to decelerate
FlightPlan FlightPlanner::plan(
  const ShipInfo& shipInfo, geometry::Point start, geometry::Point end)
{
  // How much time will it take to turn to 180 degrees?
  const double t_turn_s = M_PI / shipInfo.nMaxTurnSpeed;

  const double a_max = shipInfo.nMaxThrust / shipInfo.nWeight;
  const double s = start.distance(end);

  // Now we need to calculate how much time should be accelerate and
  // decelerate to reach the target point with 0 velocity.

  const double t_acc =
    (std::pow(t_turn_s * t_turn_s + 4 * s / a_max, 0.5) - t_turn_s) / 2;

  const uint32_t nBurnMs = static_cast<uint32_t>(std::llround(t_acc * 1000));
  const geometry::Vector forward = start.vectorTo(end).normalized();
  const geometry::Vector backward = -forward;

  FlightPlan plan;
  plan.maneuvers.reserve(4);
  plan.addTurn(forward, shipInfo.nMaxTurnSpeed);
  plan.addHoverEngineBurn(shipInfo.nMaxThrust, nBurnMs, forward);
  plan.addTurn(backward, shipInfo.nMaxTurnSpeed);
  plan.addHoverEngineBurn(shipInfo.nMaxThrust, nBurnMs, backward);

  return plan;
}

// Cancels velocity with two maneuvers: turn to face opposite the velocity,
// then burn. A full-thrust burn that would finish sooner than
// minimalBurnDurationMs is stretched by lowering thrust, so the impulse
// stays the same and the burn is at least that long.
FlightPlan FlightPlanner::stop(
  const ShipInfo& shipInfo, const geometry::Vector& velocity,
  double minimalBurnDurationMs)
{
  const double speed = velocity.getLength();
  if (speed == 0) {
    return {};
  }

  const double a_max          = shipInfo.nMaxThrust / shipInfo.nWeight;
  const double tAtMaxThrustMs = speed / a_max * 1000.0;

  double thrust = shipInfo.nMaxThrust;
  double durationMs = tAtMaxThrustMs;
  if (durationMs < minimalBurnDurationMs) {
    durationMs = minimalBurnDurationMs;
    const double durationSec = durationMs / 1000.0;
    thrust = shipInfo.nWeight * speed / durationSec;
  }

  const geometry::Vector brake = (-velocity).normalized();

  FlightPlan plan;
  plan.maneuvers.reserve(2);
  plan.addTurn(brake, shipInfo.nMaxTurnSpeed);
  plan.addHoverEngineBurn(
    static_cast<uint64_t>(std::llround(thrust)),
    static_cast<uint32_t>(std::llround(durationMs)),
    brake);
  return plan;
}

}

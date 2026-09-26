# TSSDK-004: Follow a flight plan and correct drift with the RCS

## Background

Following a flight plan today means sending each planned turn and each planned
burn, then waiting until the plan's time runs out. That follower was written
for a ship that turns and burns along its nose. It does not use the RCS. It
never reads the ship's position and velocity, so it cannot tell that the ship
has left the trajectory the plan describes.

A late command or any other disturbance leaves the ship off that trajectory.
The follower still reports success when the clock reaches the end. Nothing
puts the ship back on the plan, and the caller gets no error when the drift
has become too large to remove.

## Require

Add a second follower, `follow_flight_plan_with_corrections`, beside the one
that exists today. The existing follower stays.

The new follower still carries out the plan. Whenever the ship's position and
velocity are updated, it computes the position and velocity the plan implies
at that moment and measures how far the actual state is from that ideal
state.

Two thresholds apply to that deviation. The failure threshold is much larger
than the correction threshold.

- At or below the correction threshold, the follower does not command an RCS
  correction.
- Above the correction threshold, and not above the failure threshold, the
  follower commands a single RCS burn that brings the current position and
  velocity to the ideal ones.
- Above the failure threshold, the follower stops and returns an error. It
  does not command a correction for that deviation.

That error means this attempt to stay on the plan has failed. Building
another plan and starting it is the caller's job.

## Acceptance

- A flight that stays within the correction threshold finishes on the plan,
  and the RCS is not used to correct it.
- When the ship drifts past the correction threshold without passing the
  failure threshold, the follower commands one RCS burn for that deviation.
  After the burn, the ship's position and velocity match the position and
  velocity the plan implies. A later drift is corrected the same way, again
  with one burn.
- When the deviation passes the failure threshold, the follower returns an
  error and stops following that plan.
- The follower that exists today still follows a plan without the RCS, and
  the flights it already reaches still arrive.

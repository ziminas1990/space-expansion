# TSSDK-003: An alternative navigation algorithm

## Background

The client builds a flight plan and then follows it. Both parts were written
for a ship that turns and burns along its nose. They do not use the RCS.

The description of a plan, the check that a plan is valid, and the builder
that produces the plan are one piece of code. Following the plan is a
separate piece. A second builder cannot share the plan and the check without
taking the current builder with them.

A second builder is needed. It should reduce the planar flight to two
independent one-dimensional problems, one along each axis. The concrete
rules of that builder will be given when the work starts.

## Require

Split planning and following so that a plan does not belong to one algorithm,
keep the current planner, and add the second planner beside it.

- Flight plans and following a plan live together under navigation utilities
  (`utils/navigation`).
- The types of a plan and the check that a plan is valid do not depend on
  which algorithm built the plan. They live in `flight_plan`, apart from
  every planner.
- The planner that exists today lives in `astra_flight_planner`. It still
  builds a plan by turning the ship and burning along the nose, without the
  RCS.
- A second planner builds a plan by solving two one-dimensional problems. It
  uses the same plan types and the same check. Its maneuver rules are
  specified when that planner is written.
- Following a plan stays one shared capability. It is not part of either
  planner.

## Acceptance

- A flight plan and the check of a plan can be used without either planner.
- The Astra planner still produces a plan the check accepts. Following that
  plan still brings the ship to the target for the flights the current
  planner already reaches.
- The second planner is separate from Astra. A plan it builds is a flight
  plan in the shared form, made from two one-dimensional problems, and the
  shared check accepts it.
- Callers that build a plan with the current algorithm and follow it still
  can. Those flights still arrive.

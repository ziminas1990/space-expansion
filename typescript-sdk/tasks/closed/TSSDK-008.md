# TSSDK-008: Monitor Shipyard and show its build in the tactical map

## Background

The tactical map monitors cargo and propulsion modules on the player's ships.
Shipyard is listed among installed modules, but its current work is not shown.

The current `IShipyard.BuildingReport` in `server/Protocol.proto` carries a
status and progress only. It does not carry the ordered ship name or blueprint
name, and there is no request for the current build when a client connects.
[SES-233](../../../docs/tasks/closed/SES-233.md) adds the monitoring stream that this
task depends on.

## Require

For each Shipyard on a player ship, follow its current build and subsequent
changes. Keep its state with that particular module through tactical-map world
updates and snapshots. Start following Shipyards already installed and ones
attached later; discard state when the module or ship leaves.

In the module widget header, show `idle`, `building X%`, or `frozen X%`.
For an active build, show the ship blueprint type, its ordered name, and a
progress bar with `Progress: X%` immediately above it. Use the same percent
for the header, label, and bar.

## Acceptance

- Opening the map during an existing build shows its current status,
  blueprint type, ordered name, and progress without waiting for a new build.
- A new build and later progress or freeze reports update the correct Shipyard
  widget. Completion returns it to `idle`. Terminal cancellation or failure
  reports also return it to `idle` if the server emits them.
- Multiple Shipyards retain independent state across world snapshots and
  live updates. Removing a module or ship removes its build state.
- The displayed percent is within 0–100 and agrees with the progress bar.
- Shipyard state comes from the monitoring command in SES-233.

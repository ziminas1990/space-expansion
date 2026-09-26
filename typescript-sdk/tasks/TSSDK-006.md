# TSSDK-006: Monitor cargo and propulsion of the player's ships

## Background

The tactical map follows each ship the player controls: where it is, which
way it faces, and what its passive scanners see. It does not follow the
ship's cargo or its propulsion.

A resource container can gain or lose resources while the map is open. An
engine can change its thrust, and an RCS can change both its thrust and the
direction of that thrust. The map keeps the last picture it had of the ship
and never hears those changes, so the ship it shows has no cargo and no
account of whether anything is firing.

## Require

For every ship the player controls, subscribe to its resource containers,
its engines, and its RCS units, and keep what those subscriptions report
as part of that ship.

Each such module is kept on its own, under its own name. A ship may have
more than one container, more than one engine, and more than one RCS.

- A resource container contributes what it holds: how much space it has,
  how much of that space is in use, and the amount of each resource.
- An engine contributes its current thrust. That thrust is applied along
  the ship's nose, so the engine has no thrust direction of its own.
- An RCS contributes its current thrust and the direction of that thrust.

The subscription starts with the state the module already has, and then
follows later changes. A module that appears on a ship already on the map
is included. A module that leaves is dropped. When the ship itself leaves,
its cargo and propulsion leave with it.

Ships the player does not control stay as they are. The map still knows
only their position and facing.

## Acceptance

- A player ship on the map includes each of its resource containers, and
  each container shows its space, how full it is, and the resources inside.
- When a container's contents change, that ship's record changes to match.
- A player ship includes each of its engines, with the thrust that engine
  is producing, and each of its RCS units, with the thrust and the
  direction of that thrust.
- When an engine's thrust changes, or an RCS changes its thrust or its
  direction, that ship's record changes to match.
- A container, engine, or RCS that appears later is added. One that is
  removed is dropped. A ship that leaves takes this data with it.
- A detected ship that the player does not control still has no cargo and
  no propulsion record.
- The map still follows ship positions, facing, and scanner contacts as
  before.

# Asteroid Miner — the IAsteroidMiner interface

Previous topic: [Asteroid Scanner](./asteroid_scanner.md)
Next topic: [Blueprints Library](./blueprints_library.md)

An asteroid miner is installed on a ship and takes resources out of an
asteroid. It places metals, ice, and silicates into a
[resource container](./resource_container.md) on the same ship. Stone stays out
of the container.

## How mining works

The miner has three parameters:

- `max_distance` — the greatest distance at which the miner can work, in
  meters. The distance is measured from the ship to the center of the asteroid
- `cycle_time_ms` — the length of one mining cycle, in milliseconds of
  [ingame time](./glossary.md#ingame-time)
- `yield_per_cycle` — the mass taken from the asteroid in one cycle, in
  kilograms

Before mining can start, the miner must be bound to a resource container on
the same ship. The binding is the container's module name.

The miner works on one asteroid at a time. Every `cycle_time_ms` it takes
resources from the asteroid, puts them into the bound container, and sends a
`mining_report`.

A chunk weighs `yield_per_cycle` kilograms. The mix of the chunk is random and
follows the asteroid's composition. The miner then places the metals, silicates,
and ice from the chunk into the bound container and leaves the stone out.
The report lists that stored mass.

The distance from the ship to the center of the asteroid must stay within
`max_distance` for the whole run.

The reports, and the messages that end the run, arrive on the session that
sent `start_mining`.

## How to connect to an asteroid miner

Asteroid miners are modules of type `"AsteroidMiner"` on the ship's
[commutator](./commutator.md). A ship may carry several of them. The module name
is the name of that particular miner; it is unique among the modules of that
commutator.

After opening a session to the ship, the client requests the ship's modules,
keeps those of type `"AsteroidMiner"`, and sends `open_tunnel` with the
miner's slot. The server returns a new session that implements the
`IAsteroidMiner` interface.

The interface has four commands:

- `specification_req` — request the miner's parameters
- `bind_to_cargo` — choose the container that receives the resources
- `start_mining` — start mining an asteroid
- `stop_mining` — stop the current run

The asteroid's identifier is the `id` a [passive scanner](./passive_scanner.md)
reports for an object whose `object_type` is `OBJECT_ASTEROID`. The same
identifier is used by an [asteroid scanner](./asteroid_scanner.md).

## The specification_req command

`specification_req` requests the miner's parameters. The server replies with
one `specification` message. Its fields are `max_distance`, `cycle_time_ms`,
and `yield_per_cycle`, described in [How mining works](#how-mining-works).

## The bind_to_cargo command

`bind_to_cargo` binds the miner to a resource container on the same ship. The
command carries the container's module name.

The server replies with `bind_to_cargo_status`:

- `SUCCESS` — the miner is bound to that container. A later `bind_to_cargo`
  can choose another container, including while mining is running. Cycles
  after the new binding use the new container
- `NOT_BOUND_TO_CARGO` — the ship has no module with that name, or the module
  is not a resource container. The previous binding stays as it was

## The start_mining command

`start_mining` starts mining. The command carries the asteroid's identifier.

The server replies at once with `start_mining_status`:

- `SUCCESS` — mining has started. `mining_report` messages will follow on
  this session
- `MINER_IS_BUSY` — this miner is already mining. The run that is already
  going continues
- `NOT_BOUND_TO_CARGO` — the miner is not bound to a container
- `ASTEROID_DOESNT_EXIST` — there is no asteroid with that identifier
- `ASTEROID_TOO_FAR` — the asteroid is farther than `max_distance`

After `SUCCESS`, each finished cycle sends one `mining_report` on this
session. The report's `items` list the resources placed in the container
during that cycle. Each item has `type` and `amount`. `amount` is the mass
placed, in kilograms.

A report can look like this:

```json
{
  "tunnelId": 1002,
  "timestamp": 241000000,
  "asteroid_miner": {
    "mining_report": {
      "items": [
        { "type": "RESOURCE_METALS", "amount": 180.5 },
        { "type": "RESOURCE_SILICATES", "amount": 420.0 },
        { "type": "RESOURCE_ICE", "amount": 75.25 }
      ]
    }
  }
}
```

The mix changes from cycle to cycle.

Mining ends for one of these reasons:

- The ship moves farther than `max_distance`. The server sends
  `start_mining_status` with `ASTEROID_TOO_FAR` on this session
- The asteroid no longer exists. The server sends `start_mining_status` with
  `ASTEROID_DOESNT_EXIST` on this session
- The container cannot take the whole of a resource. That resource's `amount`
  in the report is the mass that fit. Resources later in the cycle are not
  stored. After the report, the server sends `mining_is_stopped` with
  `NO_SPACE_AVAILABLE`
- The client sends `stop_mining`. The server sends `mining_is_stopped` with
  `INTERRUPTED_BY_USER` on this session

## The stop_mining command

`stop_mining` stops the current run. The value of the bool is ignored.

The server replies with `stop_mining_status` on the session that sent the
command:

- `SUCCESS` — mining has stopped. The session that started the run receives
  `mining_is_stopped` with `INTERRUPTED_BY_USER`
- `MINER_IS_IDLE` — the miner was not mining. No `mining_is_stopped` follows

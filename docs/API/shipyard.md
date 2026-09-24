# Shipyard — the IShipyard interface

Previous topic: [Blueprints Library](./blueprints_library.md)
Next topic: [Game](./game.md)

A shipyard is installed on a ship and builds new ships for the player. The
finished ship is attached to the player's [commutator](./commutator.md), in the
same place, with the same velocity, and facing the same way as the ship that
carries the shipyard.

## How a build works

The shipyard has one parameter:

- `labor_per_sec` — how much labor the shipyard produces per second of
  [ingame time](./glossary.md#ingame-time)

A build follows a ship blueprint. The blueprint's expenses, including the
expenses of its modules, are the cost. Read them through the
[blueprints library](./blueprints_library.md). `RESOURCE_LABOR` in those
expenses is the amount of labor the build takes. The other resources are
taken from a container.

The build lasts `labor / labor_per_sec` seconds of ingame time, while the
container can supply the materials. As the build advances, the shipyard draws
those materials from the container in proportion to the progress. One step
draws every material together. If the container cannot supply every material
for the step, the step draws nothing and progress stays where it is.

Before a build can start, the shipyard must be bound to a resource container
on the same ship. The binding is the container's module name.

The shipyard builds one ship at a time. About every 500 milliseconds of
ingame time it sends a `building_report`. The report's `progress` is the
fraction of the build that is done, from 0 to 1.

## How to connect to a shipyard

Shipyards are modules of type `"Shipyard"` on the ship's
[commutator](./commutator.md). A ship may carry several of them. The module name
is the name of that particular shipyard; it is unique among the modules of
that commutator.

After opening a session to the ship, the client requests the ship's modules,
keeps those of type `"Shipyard"`, and sends `open_tunnel` with the shipyard's
slot. The server returns a new session that implements the `IShipyard`
interface.

The interface has three commands:

- `specification_req` — request the shipyard's parameter
- `bind_to_cargo` — choose the container that supplies the materials
- `start_build` — start building a ship

## The specification_req command

`specification_req` requests the shipyard's parameter. The server replies with
one `specification` message. Its field is `labor_per_sec`, described in
[How a build works](#how-a-build-works).

## The bind_to_cargo command

`bind_to_cargo` binds the shipyard to a resource container on the same ship.
The command carries the container's module name. An empty string clears the
binding.

The server replies with `bind_to_cargo_status`:

- `SUCCESS` — the shipyard is bound to that container, or the binding was
  cleared. A later `bind_to_cargo` can choose another container, including
  while a build is running. Further progress uses the new container
- `CARGO_NOT_FOUND` — the ship has no module with that name, or the module is
  not a resource container. The previous binding stays as it was

## The start_build command

`start_build` starts a build. The command has these fields:

- `blueprint_name` — the ship blueprint, in the form described in the
  [blueprints library](./blueprints_library.md), such as `Ship/MiningDrone`
- `ship_name` — the name to give the ship. An empty name is replaced with the
  blueprint name, such as `Ship/MiningDrone`

The server replies with `building_report` on the session that sent the
command. The report has `status` and `progress`:

- `BUILD_STARTED` — the build has started. `progress` is 0. Further reports
  follow on this session
- `SHIPYARD_IS_BUSY` — this shipyard is already building a ship. That build
  continues
- `CARGO_NOT_FOUND` — the shipyard is not bound to a container
- `BLUEPRINT_NOT_FOUND` — the player has no such ship blueprint, or one of
  the module blueprints it depends on is missing
- `INTERNAL_ERROR` — the server could not start the build

While the build runs, a `building_report` arrives about every 500 milliseconds
of ingame time:

- `BUILD_IN_PROGRESS` — the build is advancing. `progress` is the fraction
  done
- `BUILD_FROZEN` — the container could not supply the materials for the next
  step. `progress` stays as it was. The build continues when the container has
  the materials again, or when `bind_to_cargo` selects a container that has
  them
- `BUILD_COMPLETE` — the ship is built. `progress` is 1

After `BUILD_COMPLETE` the server sends `building_complete`:

- `ship_name` — the name the ship actually received
- `slot_id` — the slot on the player's commutator where the ship was attached

Open a tunnel to that slot to use the new ship. `ship_name` can differ from
the `ship_name` in `start_build`. The rule is in [Ship names](#ship-names).

For example, build `Ship/MiningDrone` under the name `Drone #1`:

```json
{
  "tunnelId": 1201,
  "timestamp": 0,
  "shipyard": {
    "start_build": {
      "blueprint_name": "Ship/MiningDrone",
      "ship_name": "Drone"
    }
  }
}
```

The server replies:

```json
{
  "tunnelId": 1201,
  "timestamp": 240000000,
  "shipyard": {
    "building_report": {
      "status": "BUILD_STARTED",
      "progress": 0
    }
  }
}
```

A later report can look like this:

```json
{
  "tunnelId": 1201,
  "timestamp": 240500000,
  "shipyard": {
    "building_report": {
      "status": "BUILD_IN_PROGRESS",
      "progress": 0.42
    }
  }
}
```

When the ship is attached:

```json
{
  "tunnelId": 1201,
  "timestamp": 242000000,
  "shipyard": {
    "building_complete": {
      "ship_name": "Drone",
      "slot_id": 4
    }
  }
}
```

## Ship names

Among the ships already attached to the player's commutator, the each ship's
name must be unique. But two players may each have a ship with the same name.

`start_build` accepts a name that is already taken. A build that has not
finished does not occupy the name, so two shipyards can build ships with the
same requested name at the same time. The name is decided when the finished
ship is attached. The ship that was already attached keeps its name. The new
ship is the one that may be renamed. A name clash does not fail the build.

If the requested name is free, the ship receives it as it was sent. If it is
taken, the server appends a suffix. When the requested name contains a space,
the suffix is a space and a number. Otherwise the suffix is a hyphen and a
number. The number is the smallest integer greater than zero that makes the
name free.

| Occupied                 | Requested  | Assigned       |
| ------------------------ | ---------- | -------------- |
| —                        | Scout      | Scout          |
| Scout                    | Scout      | Scout-I        |
| Scout, Scout-I           | Scout      | Scout-II       |
| Scout-I                  | Scout      | Scout          |
| Scout-I                  | Scout-I    | Scout-I-I      |
| Sweet Home               | Sweet Home | Sweet Home I   |
| Sweet Home, Sweet Home I | Sweet Home | Sweet Home II  |
| Drone #1                 | Drone #1   | Drone #1 I     |

`building_complete.ship_name` is the name to use. After a ship is destroyed,
its name is free again.

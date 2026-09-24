# Ship - controlling a ship

Previous topic: [ICommutator](./commutator.md)
Next topic: [System Clock](./system_clock.md)

A ship is an object that represents a ship in the game. A ship is a physical
object that has a position in the game world and can carry various equipment
(modules) on board.

## How to get the list of available ships

All ships are connected to the player's commutator as modules of type `"Ship"`.

To get the list of available ships, the client, after connecting to the game
(see [IAccessPanel](access-panel.md)) and opening a session to the commutator
(see [ICommutator](commutator.md)), requests the list of modules on the
commutator and filters it, keeping only modules of type `"Ship"`.

Using the `open_tunnel` command, the client can open a connection to a ship
and interact with it.

## Which interfaces does a ship implement

A ship implements several interfaces at once:

- `IShip` — for getting the ship's state
- `INavigation` — for getting information about the ship's position
- `ICommutator` — the ship's commutator, for connecting to the ship's equipment

This multiple implementation means that commands from all of the listed
interfaces can be sent on a session to the ship, and they will be handled
correctly.

The `ICommutator` interface is already familiar from the
[previous chapter](commutator.md). The ship implements this interface because
it carries equipment. Through the commands of the `ICommutator` interface the
client can explore which modules are installed on the ship and open connections
to them.

## How to get the ship's state and position

To get and monitor the ship's state, and to turn it, use the commands of the
`IShip` interface:

- `state_req` — request the ship's state
- `monitor` — start monitoring the ship's state
- `specification_req` — request the maximum rotation speed
- `rotate` — turn the nose toward a direction

In response to a `state_req` request the server sends a `state` message that
describes the ship's current state. The response contains:

- `position` — the ship's position in the game world, including its velocity
- `weight` — the ship's mass, in kilograms. The field has the form
  `{ "value": ... }`, because in the protocol it is an `OptionalDouble`
  message, not a single number
- `orientation` — the direction from the ship's center toward its nose, in
  global coordinates. `x` and `y` are the components of a unit vector. A
  `state` read while the ship is turning reports the nose at that moment.
  The saved ship state always includes this direction

To receive the ship's state regularly, the client sends a `monitor` command
with an update interval in milliseconds of
[ingame time](./glossary.md#ingame-time). The session starts receiving `state`
messages, the same as the response to `state_req` (see
[monitoring](monitoring_concept.md)).

In response to a `monitor` command the server immediately sends a `state`
message with the ship's current state and then keeps sending it at the
specified interval.

For example, `monitor=200` means a `state` message about every 200 milliseconds
of ingame time. The `monitor` value must be in the range from 100 to 60000
milliseconds.

An alternative way to get the ship's position without requesting its full
state is the `position_req` command of the `INavigation` interface. In response
the server sends a `position` message. It has the same fields as
`state.position`.

## In what format is the ship's position sent

The ship's position is sent in the `state.position` field. This field has the
following fields:

- `x` and `y` — the ship's global coordinates, in meters
- `vx` and `vy` — the ship's velocity vector, in meters per second

The `position` field itself does not say the time for which the state was
obtained. That moment is recorded in the `timestamp` field of the `Message`:
it is the [ingame time](./glossary.md#ingame-time) at which the ship had the
given coordinates and velocity.

For example, the user requested the ship's state:

```json
{
  "session_id": 445,  // session to the ship
  "ship": {
    "state_req": true
  }
}
```

In response the server sends this message:

```json
{
  "session_id": 445,
  "timestamp": 238221334,  // ingame time mark, in microseconds
  "ship": {
    "state": {
      "weight": { "value": 100500 },
      "position": {
        "x": 1000,
        "y": 2000,
        "vx": 10,
        "vy": 20
      },
      "orientation": {
        "x": 1,
        "y": 0
      }
    }
  }
}
```

## The specification_req command

`specification_req` requests the ship's limits and size. The server replies
with one `specification` message:

- `max_rotation_speed` — the fastest the ship can turn, in radians per
  second
- `radius` — the ship's size, in meters. It is the hull radius

Both numbers are the ones set on that ship's blueprint. The
[blueprints library](./blueprints_library.md) publishes the same values as
properties of the ship blueprint.

## The rotate command

`rotate` turns the nose toward a direction. The server answers at once with
`rotate_ack` and then sends nothing further about that turn. The turn runs
to completion on the server. There is no separate message when the nose
reaches the target.

The command has these fields:

- `x` and `y` — the target direction, in global coordinates. The server uses
  this pair only as a direction. A vector `(2, 2)` and a vector `(1, 1)`
  select the same nose. If both components are zero, the server acknowledges
  the command and leaves the current turn unchanged
- `speed` — how fast to turn, in radians per second

The ship turns at that speed along the shorter arc until the nose matches
the target. The speed used does not exceed `max_rotation_speed`. A requested
speed above the maximum is carried out at the maximum. A speed of zero or
less acknowledges the command, leaves the nose where it is, and cancels a
turn that is already in progress.

A `rotate` that arrives during a turn replaces that turn. The nose stays
where it is at that moment and then turns toward the new target.


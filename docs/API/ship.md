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

To get and monitor the ship's state, use the commands of the `IShip` interface:

- `state_req` — request the ship's state
- `monitor` — start monitoring the ship's state

In response to a `state_req` request the server sends a `state` message that
describes the ship's current state. The response contains:

- `position` — the ship's position in the game world, including its velocity
- `weight` — the ship's mass, in kilograms. The field has the form
  `{ "value": ... }`, because in the protocol it is an `OptionalDouble`
  message, not a single number

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
      }
    }
  }
}
```

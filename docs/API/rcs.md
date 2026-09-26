# RCS — the IRCS interface

Previous topic: [System Clock](./system_clock.md)
Next topic: [Hover Engine](./hover_engine.md)

RCS is a set of thrusters built into the hull. Working together, they push in
any direction. This is the low-power propulsion system. The main engine, which
thrusts only along the nose and is how the player chooses how hard the ship
burns, is the [Hover Engine](./hover_engine.md).

The thrust command sets a direction and does not set a magnitude. While the
thrusters are running, the force is this module's maximum thrust along that
direction.

Thrust is a force. While the thrusters are running, that force produces
acceleration according to Newton's second law. The force stays at the maximum
until the commanded duration passes, or until a later command sets another
direction, including a zero vector.

The direction is in global coordinates, the same frame as the ship's position.
If the ship turns while the thrusters are producing thrust, the force does not
turn with the ship. It keeps the direction that was set.

The current implementation:

1. it can change the thrust direction instantly
2. it needs no working mass
3. the ship's mass stays the same while the thrusters are running

## How to connect to RCS

RCS modules are of type `"RCS"` on the ship's
[commutator](./commutator.md). A ship may carry several of them. The module name
is the name of that particular set of thrusters; it is unique among the modules
of that commutator.

After opening a session to the ship, the client requests the ship's modules,
keeps those of type `"RCS"`, and sends `open_tunnel` with that module's slot.
The server returns a new session, that implements the `IRCS` interface.

The interface has four commands:

- `specification_req` — request the thrust limit
- `change_thrust` — set the thrust direction and how long it lasts
- `thrust_req` — request the current thrust
- `monitor` — receive the thrust whenever it changes

## The specification_req command

`specification_req` requests the thrusters' parameters. The server replies with
one `specification` message, that has the following fields:

- `max_thrust` — the largest thrust this set can produce, in newtons.

## The change_thrust command

`change_thrust` sets the direction of the thrust and how long the thrusters
keep it. The server sends no reply on this session. The command has these
fields:

- `x` and `y` — the direction of the force, in global coordinates. The server
  uses this pair only as a direction. A vector `(3, 4)` and a vector `(0.6, 0.8)`
  select the same direction, and both produce the same force. The length of
  the vector does not change the force.
- `duration_ms` — how long the thrusters keep this thrust, in milliseconds of
  [ingame time](./glossary.md#ingame-time)

While the thrusters are running, the force is `max_thrust` along that
direction.

The direction does not change when the ship rotates. The force stays in the
global direction that was set.

The duration is counted from the moment the server applies the command. When
it has passed, the server sets the thrust to zero.

A zero vector (`x` and `y` both `0`) produces no thrust. In that case the
server does not use `duration_ms`.

A later `change_thrust` replaces the current thrust and starts its own
duration.

As with every other command, a `timestamp` in the future delays the command
until [ingame time](./glossary.md#ingame-time) passes that mark. The duration
starts when the command is applied.

For example, the direction `(3, 4)` for 500 milliseconds of ingame time, on a
module whose `max_thrust` is 100 newtons:

```json
{
  "tunnelId": 601,
  "timestamp": 0,
  "rcs": {
    "change_thrust": {
      "x": 3,
      "y": 4,
      "duration_ms": 500
    }
  }
}
```

The applied force has magnitude 100 along the unit vector `(0.6, 0.8)`, so its
components are 60 and 80 newtons. A command with `x` and `y` of `6` and `8`,
or of `0.6` and `0.8`, applies that same force. To read the vector back, use
`thrust_req` or `monitor`.

## The thrust_req command

`thrust_req` requests the thrust that is applied right now. The server replies
at once with one `thrust` message:

- `x` and `y` — the components of the applied force, in newtons
- `thrust` — the magnitude of that force, in newtons

These components are the force itself. They are the direction from
`change_thrust` scaled to `max_thrust`. While the thrusters are running,
`thrust` is that maximum. When they are not, `thrust` is `0`.

While the thrusters from the example above are running, the response is:

```json
{
  "tunnelId": 602,
  "timestamp": 240000000,  // ingame time, in microseconds
  "rcs": {
    "thrust": {
      "x": 60,
      "y": 80,
      "thrust": 100
    }
  }
}
```

After the duration ends, or after a `change_thrust` with a zero vector, the
same request returns `x`, `y`, and `thrust` all equal to `0`.

## The monitor command

`monitor` subscribes the session to `thrust` messages, the same messages that
`thrust_req` returns. The command carries no interval: the server sends a
message when the thrust changes. See the
[monitoring_concept.md](./monitoring_concept.md) to know more about monitoring.

The server sends the current thrust at once, as a snapshot. After that it
sends `thrust` again:

- when a `change_thrust` is applied, including a command whose vector is the
  same as the current one, and including a command delayed by `timestamp`
- when `duration_ms` has passed and the thrust becomes zero

Several sessions can monitor one RCS module at the same time.

For example:

```json
{
  "tunnelId": 603,
  "timestamp": 0,
  "rcs": {
    "monitor": true
  }
}
```

The server replies at once with the current thrust:

```json
{
  "tunnelId": 603,
  "timestamp": 240000100,  // ingame time, in microseconds
  "rcs": {
    "thrust": {
      "x": 0,
      "y": 0,
      "thrust": 0
    }
  }
}
```

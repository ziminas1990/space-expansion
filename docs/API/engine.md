# Engine — the IEngine interface

Previous topic: [System Clock](./system_clock.md)
Next topic: [Passive Scanner](./passive_scanner.md)

An engine is installed on a ship and can serve as a source of thrust. Thrust is
a force vector that the engine applies to the ship while it is running. That
thrust produces acceleration according to Newton's second law.

The current implementation is an ideal engine:

1. it can change the thrust vector instantly
2. it needs no working mass, so it is not a reaction engine
3. the ship's mass stays the same while the engine is running

## How to connect to an engine

Engines are modules of type `"Engine"` on the ship's
[commutator](./commutator.md). A ship may carry several of them. The module name
is the name of that particular engine; it is unique among the modules of that
commutator.

After opening a session to the ship, the client requests the ship's modules,
keeps those of type `"Engine"`, and sends `open_tunnel` with the engine's slot.
The server returns a new session, that implements the `IEngine` interface.

The interface has four commands:

- `specification_req` — request the engine's limit
- `change_thrust` — set the thrust
- `thrust_req` — request the current thrust
- `monitor` — receive the thrust whenever it changes

## The specification_req command

`specification_req` requests the engine's parameters. The server replies  with
one `specification` message, that has the following fields:

- `max_thrust` — the largest thrust this engine can produce, in newtons

For example:

```json
{
  "tunnelId": 600,
  "timestamp": 0,
  "engine": {
    "specification_req": true
  }
}
```

The server replies:

```json
{
  "tunnelId": 600,
  "timestamp": 238221334,  // ingame time, in microseconds
  "engine": {
    "specification": {
      "max_thrust": 10000
    }
  }
}
```

## The change_thrust command

`change_thrust` sets the thrust. The server sends NO reply on this session.
The command has these fields:

- `x` and `y` — the direction of the force. The server uses this pair only as
  a direction. A vector `(3, 4)` and a vector `(0.6, 0.8)` select the same
  direction.
- `thrust` — the magnitude of the force, in newtons
- `duration_ms` — how long the engine keeps this thrust, in milliseconds of
  [ingame time](./glossary.md#ingame-time)

The duration is counted from the moment the server applies the command. When
it has passed, the server sets the thrust to zero.

A `thrust` of `0` sets the thrust to zero at once. In that case the server
does not use `x`, `y`, or `duration_ms`.

If `thrust` is greater than `max_thrust`, the server uses `max_thrust`.

A later `change_thrust` replaces the current thrust and starts its own
duration.

As with every other command, a `timestamp` in the future delays the command
until [ingame time](./glossary.md#ingame-time) passes that mark. The duration
starts when the command is applied.

For example, thrust of 100 newtons in the direction `(3, 4)` for 500
milliseconds of ingame time:

```json
{
  "tunnelId": 601,
  "timestamp": 0,
  "engine": {
    "change_thrust": {
      "x": 3,
      "y": 4,
      "thrust": 100,
      "duration_ms": 500
    }
  }
}
```

The applied force has magnitude 100 along the unit vector `(0.6, 0.8)`, so its
components are 60 and 80 newtons. To read that vector back, use `thrust_req`
or `monitor`.

## The thrust_req command

`thrust_req` requests the thrust that is applied right now. The server replies
at once with one `thrust` message:

- `x` and `y` — the components of the applied force, in newtons
- `thrust` — the magnitude of that force, in newtons

These components are the force itself. They are the direction from
`change_thrust` scaled to the applied magnitude.

While the engine from the example above is running, the response is:

```json
{
  "tunnelId": 602,
  "timestamp": 240000000,  // ingame time, in microseconds
  "engine": {
    "thrust": {
      "x": 60,
      "y": 80,
      "thrust": 100
    }
  }
}
```

After the duration ends, or after a `change_thrust` with `thrust` equal to
`0`, the same request returns `x`, `y`, and `thrust` all equal to `0`.

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

Several sessions can monitor one engine at the same time.

For example:

```json
{
  "tunnelId": 603,
  "timestamp": 0,
  "engine": {
    "monitor": true
  }
}
```

The server replies at once with the current thrust:

```json
{
  "tunnelId": 603,
  "timestamp": 240000100,  // ingame time, in microseconds
  "engine": {
    "thrust": {
      "x": 0,
      "y": 0,
      "thrust": 0
    }
  }
}
```

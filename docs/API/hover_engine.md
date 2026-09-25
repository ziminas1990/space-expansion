# Hover Engine — the IHoverEngine interface

Previous topic: [RCS](./rcs.md)
Next topic: [Passive Scanner](./passive_scanner.md)

The hover engine is the ship's main engine. It pushes along the nose, so the
player aims it by turning the ship. The command sets a magnitude and does not
take a direction.

Thrust is a force. While the engine is running, that force produces acceleration
according to Newton's second law. The force stays at the commanded magnitude
until the commanded duration passes, or until a later command sets another
magnitude, including zero. If the ship turns while the engine is producing
thrust, the force turns with the nose. The magnitude stays the one that was
set, and it does not exceed the maximum.

The current implementation:

1. it can change the thrust magnitude at once
2. it needs no working mass
3. the ship's mass stays the same while the engine is running

## How to connect to a hover engine

Hover engines are of type `"HoverEngine"` on the ship's
[commutator](./commutator.md). A ship may have only one hover engine.

After opening a session to the ship, the client requests the ship's modules,
finds the module of type `"HoverEngine"`, and sends `open_tunnel` with that module's slot. The server returns a new session, that implements the `IHoverEngine` interface.

The interface has four commands:

- `specification_req` — request the thrust limit
- `change_thrust` — set the thrust magnitude and how long it lasts
- `thrust_req` — request the thrust that is applied
- `monitor` — receive the thrust whenever it changes

## The specification_req command

`specification_req` requests the engine's parameters. The server replies with
one `specification` message, that has the following fields:

- `max_thrust` — the largest thrust this engine can produce, in newtons. The
  specification does not describe a way to aim the engine.

## The change_thrust command

`change_thrust` sets the thrust magnitude and how long the engine keeps it.
The server sends no reply on this session. The command has these fields:

- `thrust` — the magnitude of the force, in newtons
- `duration_ms` — how long the engine keeps this thrust, in milliseconds of
  [ingame time](./glossary.md#ingame-time)

The force is applied along the ship's current orientation. If the ship turns
while this thrust is applied, the force turns with the nose.

The duration is counted from the moment the server applies the command. When
it has passed, the server sets the thrust to zero.

A `thrust` of `0` sets the thrust to zero at once. In that case the server
does not use `duration_ms`.

If `thrust` is greater than `max_thrust`, the server uses `max_thrust`.

A later `change_thrust` replaces the current thrust and starts its own
duration.

As with every other command, a `timestamp` in the future delays the command
until [ingame time](./glossary.md#ingame-time) passes that mark. The duration
starts when the command is applied.

For example, a thrust of 1000 newtons for 500 milliseconds of ingame time:

```json
{
  "tunnelId": 701,
  "timestamp": 0,
  "hover_engine": {
    "change_thrust": {
      "thrust": 1000,
      "duration_ms": 500
    }
  }
}
```

## The thrust_req command

`thrust_req` requests the thrust magnitude that is applied right now. The server
replies at once with one `thrust` message: the magnitude in newtons.

## The monitor command

`monitor` subscribes the session to `thrust` messages, the same magnitude that
`thrust_req` returns. The command carries no interval: the server sends a
message when that magnitude changes. See
[monitoring_concept.md](./monitoring_concept.md) to know more about monitoring.

The server sends the current thrust at once, as a snapshot. After that it
sends `thrust` again:

- when a `change_thrust` is applied, including a command whose magnitude is
  the same as the current one, and including a command delayed by `timestamp`
- when `duration_ms` has passed and the thrust becomes zero

Turning the ship does not send a message: the reported value is only the
magnitude, and the magnitude stays the one that was set.

Several sessions can monitor one hover engine at the same time.

# System Clock — the ISystemClock interface

Previous topic: [Ship](./ship.md)
Next topic: [RCS](./rcs.md)

The system clock is a virtual player module. Through it the client can:

- obtain the [ingame time](./glossary.md#ingame-time) and the physical time on
  the server that corresponds to it
- monitor the passage of ingame time
- set a timer or an alarm

## How to connect to the system clock

A module of type `"SystemClock"` with the name `"SystemClock"` is always
connected to the player's commutator, and there is exactly one. The module
implements the `ISystemClock` interface, which has four commands:

- `time_req` — request the current time
- `wait_until` — wait until an absolute ingame-time mark
- `wait_for` — wait for a given span of ingame time
- `monitor` — receive the current time regularly

## Two times in one response

Every system-clock response carries two different marks.

The `timestamp` of the top-level `Message` is the
[ingame time](./glossary.md#ingame-time) in microseconds. It is the simulation
moment at which the server formed the response. The same field is used in the
server's other responses.

The `time` or `ring` field inside `ISystemClock` carries the **real time** in
microseconds since the server started. It follows the clock of the machine the
server runs on, and it does not slow down when the simulation falls behind
real time.

The `wait_until`, `wait_for`, and `monitor` commands count their deadline in
ingame time. The `time` and `ring` fields still hold the real time of the
moment when the server sent the response.

## The time_req command

`time_req` requests the current time. The server replies at once with a single
`time` message.

For example, the client requests the time:

```json
{
  "tunnelId": 512,
  "timestamp": 0,
  "system_clock": {
    "time_req": true
  }
}
```

The server replies:

```json
{
  "tunnelId": 512,
  "timestamp": 238221334,  // ingame time, in microseconds
  "system_clock": {
    "time": 3750100882  // real time, in microseconds
  }
}
```

Here `timestamp` is the ingame time of the response, in microseconds, and
`time` is the real time since the server started, also in microseconds.

## The wait_until and wait_for commands

These commands set an alarm or a timer. The server does not reply at once.
When the deadline arrives, it sends a single `ring` message.

`wait_until` sets an absolute ingame-time mark, in microseconds. The `ring`
message is sent on the [simulation tick](./glossary.md#simulation-ticks) on
which ingame time first exceeded that mark.

`wait_for` sets a duration in microseconds of ingame time. The server counts
it from the ingame time at the moment the command is received. A `wait_for`
value of `200000` means "send `ring` after 200 milliseconds of ingame time".

The `ring` field holds the real time at the moment the alarm fires. The ingame
time of that moment is recorded in the message `timestamp`.

Example of `wait_until`. The client asks to be woken at the 500 millisecond
mark of ingame time:

```json
{
  "tunnelId": 513,
  "timestamp": 0,
  "system_clock": {
    "wait_until": 500000
  }
}
```

When ingame time reaches that mark, the server sends:

```json
{
  "tunnelId": 513,
  "timestamp": 500012,
  "system_clock": {
    "ring": 501880
  }
}
```

`timestamp` here may be slightly greater than the requested mark: it is the
ingame time of the tick on which ingame time first exceeded the mark.

One session carries one wait. Until `ring` arrives, leave the session free of
other commands.

Several waits can run at the same time when they are requested on different
sessions.

## The monitor command

`monitor` subscribes the session to regular `time` messages, the same messages
that arrive in response to `time_req`. The argument is an interval in
milliseconds of ingame time. The general subscription scheme is described in
[monitoring_concept.md](./monitoring_concept.md).

The server does not send the time at once. The first `time` message arrives
after one interval, and further messages follow at the same step.

The server raises an interval below 20 milliseconds to 20. There is no upper
bound. The command returns no acknowledgement and no error code.

Sending `monitor` again on the same session sets a new interval: the next
`time` arrives one new interval after the moment the server accepted the
command. The stream stops only when the session is closed.

For example, `monitor` with an interval of 100 milliseconds of ingame time:

```json
{
  "tunnelId": 514,
  "timestamp": 0,
  "system_clock": {
    "monitor": 100
  }
}
```

About 100 milliseconds of ingame time later, and then at the same step, the
server sends:

```json
{
  "tunnelId": 514,
  "timestamp": 410200100,
  "system_clock": {
    "time": 410350440
  }
}
```

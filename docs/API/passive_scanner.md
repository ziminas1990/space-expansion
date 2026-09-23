# Passive Scanner — the IPassiveScanner interface

Previous topic: [Engine](./engine.md)
Next topic: [Resource Container](./resource_container.md)

A passive scanner is installed on a ship and can serve as a source of
information about objects around that ship.

## How does passive scanner work

The scanner constantly observes the space around the ship and sends
notifications about every object it observes.

It observes asteroids and ships of other players that are inside its scanning
radius. Ships that belong to the same player are left out.

The scanner has two parameters:

- `scanning_radius_km` — the scanning radius, in kilometers. Objects farther
  from the ship than this are not observed
- `max_update_time_ms` — the notification interval, in milliseconds of
  [ingame time](./glossary.md#ingame-time), for an object at the edge of the
  scanning radius

The closer an object is to the ship, the more often the scanner sends a
notification for it. The farther the object is, the less often.

The interval until the next notification, in the same units as
`max_update_time_ms`, is:

```
dt = max_update_time_ms * (distance / scanning_radius) ^ 0.7
```

`distance` is the distance from the ship to the object. `scanning_radius` is
`scanning_radius_km` expressed in the same units. At the edge of the radius the
ratio is 1, so `dt` equals `max_update_time_ms`.

The formula above gives an approximate interval. The actual interval can differ
from it depending on how fast the target is approaching the ship or moving
away from it.

If the interval is shorter than 50 milliseconds, the scanner uses 50
milliseconds.

## How to connect to a passive scanner

Passive scanners are modules of type `"PassiveScanner"` on the ship's
[commutator](./commutator.md). A ship may carry several of them. The module name
is the name of that particular scanner; it is unique among the modules of that
commutator.

After opening a session to the ship, the client requests the ship's modules,
keeps those of type `"PassiveScanner"`, and sends `open_tunnel` with the
scanner's slot. The server returns a new session, that implements the
`IPassiveScanner` interface.

The interface has two commands:

- `specification_req` — request the scanner's parameters
- `monitor` — receive reports about nearby objects

## The specification_req command

`specification_req` requests the scanner's parameters. The server replies with
one `specification` message. Its fields are `scanning_radius_km` and
`max_update_time_ms`, described in
[How does passive scanner work](#how-does-passive-scanner-work).

For example:

```json
{
  "tunnelId": 701,
  "timestamp": 0,
  "passive_scanner": {
    "specification_req": true
  }
}
```

The server replies:

```json
{
  "tunnelId": 701,
  "timestamp": 238221334,
  "passive_scanner": {
    "specification": {
      "scanning_radius_km": 50,
      "max_update_time_ms": 2000
    }
  }
}
```

## The monitor command

`monitor` subscribes the session to reports. See
[monitoring_concept.md](./monitoring_concept.md) to know more about monitoring.

The server replies with `monitor_ack`:

- `true` — the subscription is accepted, and `update` messages will follow
- `false` — the scanner already has 8 monitoring sessions, and this one is
  rejected

Several sessions can monitor one scanner at the same time, up to 8. Each of
them receives the same reports.

An `update` is not sent at the moment of subscription. Later the server sends
`update` messages as object reports become due. One message contains at most
16 objects. When more objects are due, they are sent in the following
messages.

Each object in `update.items` has these fields:

- `object_type` — `OBJECT_ASTEROID` or `OBJECT_SHIP`
- `id` — the identifier of that asteroid or ship. The same object keeps the
  same `id` in later reports
- `x` and `y` — position, in meters
- `vx` and `vy` — velocity, in meters per second
- `r` — radius, in meters

`x`, `y`, `vx`, and `vy` have the same meaning as the ship's position (see
[Ship](./ship.md)). The `timestamp` of the message is the ingame time at which
the objects had these coordinates and this velocity.

For example, the client starts monitoring:

```json
{
  "tunnelId": 702,
  "timestamp": 0,
  "passive_scanner": {
    "monitor": true
  }
}
```

The server replies:

```json
{
  "tunnelId": 702,
  "timestamp": 238300000,
  "passive_scanner": {
    "monitor_ack": true
  }
}
```

A later report can look like this:

```json
{
  "tunnelId": 702,
  "timestamp": 240500000,
  "passive_scanner": {
    "update": {
      "items": [
        {
          "object_type": "OBJECT_ASTEROID",
          "id": 17,
          "x": 12000,
          "y": -3400,
          "vx": 1.5,
          "vy": 0.2,
          "r": 12
        }
      ]
    }
  }
}
```

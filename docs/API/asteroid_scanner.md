# Asteroid Scanner — the IAsteroidScanner interface

Previous topic: [Resource Container](./resource_container.md)
Next topic: [Asteroid Miner](./asteroid_miner.md)

An asteroid scanner is installed on a ship and reports the mass and the
composition of an asteroid.

An asteroid contains metals, ice, silicates, and stone. The scanner reports
the mass of the whole asteroid and the share of the mass that is metals, ice,
and silicates. The rest of the mass is useless stone.

## How a scan works

The scanner has two parameters:

- `max_distance` — the greatest distance at which the scanner can examine an
  asteroid, in meters. The distance is measured from the ship to the center of
  the asteroid
- `scanning_time_ms` — how long the scanner takes to examine one hectare of
  the asteroid's surface, in milliseconds of
  [ingame time](./glossary.md#ingame-time). A hectare is a square of 100 by
  100 meters

The scan examines the whole surface. Its duration, in the same units as
`scanning_time_ms`, is:

```
duration_ms = scanning_time_ms * (4 * π * radius² / 10000)
```

`radius` is the asteroid's radius in meters. The factor `4 * π * radius²` is
the surface area of the asteroid.

The duration is counted from the moment the server applies `scan_asteroid`.
For the whole scan, the distance from the ship to the center of the asteroid
must stay within `max_distance`.

A scanner runs one scan at a time.

## How to connect to an asteroid scanner

Asteroid scanners are modules of type `"AsteroidScanner"` on the ship's
[commutator](./commutator.md). A ship may carry several of them. The module name
is the name of that particular scanner; it is unique among the modules of that
commutator.

After opening a session to the ship, the client requests the ship's modules,
keeps those of type `"AsteroidScanner"`, and sends `open_tunnel` with the
scanner's slot. The server returns a new session that implements the
`IAsteroidScanner` interface.

The interface has two commands:

- `specification_req` — request the scanner's parameters
- `scan_asteroid` — scan one asteroid

The asteroid's identifier comes from a passive scanner report. It is the `id`
of an object whose `object_type` is `OBJECT_ASTEROID`.

Hint: you can use the [passive scanner](./passive_scanner.md) to find asteroids.

## The specification_req command

`specification_req` requests the scanner's parameters. The server replies with
one `specification` message. Its fields are `max_distance` and
`scanning_time_ms`, described in [How a scan works](#how-a-scan-works).

## The scan_asteroid command

`scan_asteroid` starts a scan. The command carries the asteroid's identifier.

The server replies at once with `scanning_status`:

- `IN_PROGRESS` — the scan has started. `scanning_finished` will follow on
  this session
- `SCANNER_BUSY` — this scanner is already scanning an asteroid. The scan
  that is already running continues
- `ASTEROID_TOO_FAR` — the asteroid is farther than `max_distance`, or there
  is no asteroid with that identifier

When the scan completes, the server sends `scanning_finished`:

- `asteroid_id` — the same identifier as in the request
- `weight` — the asteroid's mass, in kilograms
- `metals_percent` — the share of the mass that is metals, from 0 to 1
- `ice_percent` — the share of the mass that is ice, from 0 to 1
- `silicates_percent` — the share of the mass that is silicates, from 0 to 1

A value of `0.15` means 15% of the mass. The three shares together with the
stone make up the whole mass.

If the ship moves beyond `max_distance` before the scan completes, or the
asteroid no longer exists, the server sends `scanning_status` with
`ASTEROID_TOO_FAR` and does not send `scanning_finished`.

For example, scan the asteroid with id `17`:

```json
{
  "tunnelId": 901,
  "timestamp": 0,
  "asteroid_scanner": {
    "scan_asteroid": 17
  }
}
```

The server replies:

```json
{
  "tunnelId": 901,
  "timestamp": 240000000,
  "asteroid_scanner": {
    "scanning_status": "IN_PROGRESS"
  }
}
```

When the scan completes:

```json
{
  "tunnelId": 901,
  "timestamp": 241256600,
  "asteroid_scanner": {
    "scanning_finished": {
      "asteroid_id": 17,
      "weight": 1000000,
      "metals_percent": 0.15,
      "ice_percent": 0.05,
      "silicates_percent": 0.8
    }
  }
}
```

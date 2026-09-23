# Resource Container — the IResourceContainer interface

Previous topic: [Passive Scanner](./passive_scanner.md)
Next topic: [Asteroid Scanner](./asteroid_scanner.md)

A resource container is installed on a ship and stores resources. It can move
those resources to another container:

- on the same ship
- on another ship of the same player
- on a ship that belongs to another player

The container has a fixed volume, in cubic meters. That volume is the most
space the stored resources can occupy. The client learns it from `content`.

The container stores three resources. `amount` is the mass of a resource, in
kilograms. The density of each resource, in kilograms per cubic meter, is
listed in [Resources](./reference_info.md#resources).

The occupied volume, in cubic meters, is the sum of `amount / density` over
the resources in the container. `content` returns that sum as `used`. A
container never accepts mass that would make `used` greater than `volume`.

## How a transfer works

A transfer moves one resource from this container to another. It is not
instant. The sending container moves at most 2000 kilograms per second of
[ingame time](./glossary.md#ingame-time).

The receiving container must open a port first. A port has two values:

- `port_id` — the identifier the server assigns when the port opens
- `access_key` — a number the receiving client chooses and passes to
  `open_port`

The sending container then sends `transfer` with that `port_id`, the same
`access_key`, and the resource to move.

While the transfer runs, the gap between the two ships must stay within 200
meters. The gap is the distance between the ships after each ship's radius is
subtracted. Containers on the same ship have a gap of 0, so a transfer between
them is allowed. If the gap becomes larger than 200 meters, the server stops
the transfer.

One container can have one outgoing transfer and one open port at the same
time. But several containers can send to the same open port at once.

## How to connect to a resource container

Resource containers are modules of type `"ResourceContainer"` on the ship's
[commutator](./commutator.md). A ship may carry several of them. The module name
is the name of that particular container; it is unique among the modules of
that commutator.

After opening a session to the ship, the client requests the ship's modules,
keeps those of type `"ResourceContainer"`, and sends `open_tunnel` with the
container's slot. The server returns a new session that implements the
`IResourceContainer` interface.

The interface has five commands:

- `content_req` — request what the container holds
- `open_port` — open a port that accepts a transfer
- `close_port` — close that port
- `transfer` — start moving a resource to an open port
- `monitor` — receive the contents whenever they change

## The content_req command

`content_req` requests the container's contents. The value of the bool is
ignored. The server replies with one `content` message:

- `volume` — the container's volume, in cubic meters
- `used` — the occupied volume, in cubic meters
- `resources` — the stored resources

Each item in `resources` has `type` and `amount`. `amount` is the mass, in
kilograms. An item is included only when its amount is greater than zero.

For example:

```json
{
  "tunnelId": 800,
  "timestamp": 0,
  "resource_container": {
    "content_req": true
  }
}
```

The server replies:

```json
{
  "tunnelId": 800,
  "timestamp": 238221334,
  "resource_container": {
    "content": {
      "volume": 1000,
      "used": 3,
      "resources": [
        { "type": "RESOURCE_METALS", "amount": 4500 },
        { "type": "RESOURCE_ICE", "amount": 1832 }
      ]
    }
  }
}
```

Here 4500 kilograms of metals occupy 1 cubic meter, and 1832 kilograms of ice
occupy another 2 cubic meters, so `used` is 3.

## The open_port command

`open_port` opens a port on this container. The command carries the
`access_key`, a `uint32` chosen by the client.

On success the server replies with `port_opened`. Its value is the `port_id`.
Another container uses that id together with the same access key in
`transfer`.

On failure the server replies with `open_port_failed`:

- `PORT_ALREADY_OPEN` — this container already has an open port
- `INTERNAL_ERROR` — the server could not allocate a port

For example:

```json
{
  "tunnelId": 801,
  "timestamp": 0,
  "resource_container": {
    "open_port": 43728
  }
}
```

The server replies:

```json
{
  "tunnelId": 801,
  "timestamp": 238300000,
  "resource_container": {
    "port_opened": 1
  }
}
```

## The close_port command

`close_port` closes the port on this container. The value of the bool is
ignored. The server replies with `close_port_status`:

- `SUCCESS` — the port was closed
- `PORT_IS_NOT_OPENED` — this container had no open port

Closing the port stops every transfer that targets it. Each sending container
then receives `transfer_finished` with `PORT_HAS_BEEN_CLOSED`.

## The transfer command

`transfer` starts a transfer from this container to an open port. The command
has these fields:

- `port_id` — the port to send to
- `access_key` — the key that was passed to `open_port` on the receiver
- `resource.type` — which resource to move
- `resource.amount` — how much mass to move, in kilograms

The server replies at once with `transfer_status`:

- `SUCCESS` — the transfer has started
- `TRANSFER_IN_PROGRESS` — this container already has an outgoing transfer
- `PORT_IS_NOT_OPENED` — there is no open port with that `port_id`
- `INVALID_ACCESS_KEY` — the access key does not match the port
- `INVALID_RESOURCE_TYPE` — the type is not a material resource (for example
  labor)

After `SUCCESS`, the server sends `transfer_report` messages on the same
session for as long as the transfer runs. Each report is a `ResourceItem`:
`type` is the resource, and `amount` is the mass moved since the previous
report, in kilograms. Reports are sent about every 200 milliseconds of
[ingame time](./glossary.md#ingame-time). The total mass moved is the sum of
these `amount` values.

The receiving container does not get these reports. It sees the incoming mass
through `content` or `monitor`.

The server sends `transfer_finished` when the transfer ends:

- `SUCCESS` — the requested mass has been moved
- `PORT_HAS_BEEN_CLOSED` — the receiving port was closed
- `PORT_TOO_FAR` — the gap between the ships became larger than 200 meters

The sum of `transfer_report.amount` up to a `SUCCESS` result is the requested
mass. Mass that was reported has left the sender and is in the receiver. Mass
that was not reported stays in the sender.

If the receiving container runs out of free volume, or the sending container no
longer holds the requested resource, the server keeps sending reports with
`amount` equal to `0`. The transfer does not stop while the session stays open
and the ships remain within transfer range.

For example, move 1000 kilograms of metals to port `1` with access key
`43728`:

```json
{
  "tunnelId": 802,
  "timestamp": 0,
  "resource_container": {
    "transfer": {
      "port_id": 1,
      "access_key": 43728,
      "resource": {
        "type": "RESOURCE_METALS",
        "amount": 1000
      }
    }
  }
}
```

The server replies:

```json
{
  "tunnelId": 802,
  "timestamp": 240000000,
  "resource_container": {
    "transfer_status": "SUCCESS"
  }
}
```

A later report can look like this:

```json
{
  "tunnelId": 802,
  "timestamp": 240201000,
  "resource_container": {
    "transfer_report": {
      "type": "RESOURCE_METALS",
      "amount": 402
    }
  }
}
```

When the 1000 kilograms have been moved:

```json
{
  "tunnelId": 802,
  "timestamp": 240500000,
  "resource_container": {
    "transfer_finished": "SUCCESS"
  }
}
```

## The monitor command

`monitor` subscribes the session to `content` messages, the same messages that
`content_req` returns. See
[monitoring_concept.md](./monitoring_concept.md) to know more about monitoring.

The server sends the current contents at once, as a snapshot. After that it
sends `content` again whenever the stored mass changes. During a transfer the
mass changes on the same schedule as the transfer reports, about every 200
milliseconds of ingame time.

Several sessions can monitor one container at the same time. Each of them
receives the same contents.

For example:

```json
{
  "tunnelId": 803,
  "timestamp": 0,
  "resource_container": {
    "monitor": true
  }
}
```

The server replies at once with the current contents:

```json
{
  "tunnelId": 803,
  "timestamp": 240000100,
  "resource_container": {
    "content": {
      "volume": 1000,
      "used": 1,
      "resources": [
        { "type": "RESOURCE_METALS", "amount": 4500 }
      ]
    }
  }
}
```

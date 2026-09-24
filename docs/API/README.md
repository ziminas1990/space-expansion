# Space Expansion API

Space Expansion is a multiplayer game with an open API. The game developers do
not provide players with a single ready-made client, because the player's task
is precisely to write a client for the game.

The game takes place in outer space. The game world is a two-dimensional world
with gravity and simple Newtonian mechanics.

Through the open API a player can connect to the game server and control the
equipment on the ships that belong to them. The best way to understand which
game mechanics the game contains is to study the equipment available to
players, its capabilities and its limitations.

The client and the server talk over ordinary UDP, exchanging messages in
[Protobuf format](https://developers.google.com/protocol-buffers).

The whole protocol is described by two protobuf files:

1. `CommonTypes.proto` — a set of data types
2. `Protocol.proto` — messages exchanged between the client and the server

## UDP message format

Each individual UDP frame carries exactly one `Message`, the top-level message
described in `Protocol.proto`. To send anything to the server, wrap it in a
`Message`. `Message` is a container for every other message (via a oneof), and
it also has several important fields:

- `tunnelId` — the session (tunnel) identifier
- `timestamp` — an [ingame time](./glossary.md#ingame-time) mark

`tunnelId` will be covered in more detail later.

The meaning of `timestamp` depends on the direction of the message. If the
message goes from the client to the server (a command), the server executes it
when ingame time has exceeded `timestamp`. Because ingame time is
[discrete](./glossary.md#simulation-ticks), the actual execution time can
differ slightly from the `timestamp` value.

If the message goes from the server to the client (a response), `timestamp`
determines the ingame time at which the response was produced.

For example, if the client requests a ship's position, the server returns the
ship's coordinates and velocity, and `timestamp` holds the exact
[ingame time](./glossary.md#ingame-time) at which the ship had those
coordinates and that velocity.

## Interfaces

It is useful to think of `Protocol.proto` not merely as a set of messages, but
as a list of interfaces, where each interface corresponds to some independent
aspect of the game. We will go through every interface of this protocol step by
step, and through them explain the corresponding game mechanics.

We start with the [IAccessPanel](access-panel.md) interface, through which the
client connects to the server.

Below is the full list of the interfaces under discussion, in the recommended
order of study:

- [IAccessPanel](access-panel.md) — how to connect to the server
- [Sessions Control](sessions-control.md) — session control
- [ICommutator](commutator.md) — the interface for connecting to devices
- [Ship](ship.md) — exploring a ship
- [System Clock](system_clock.md) — the system clock
- [Engine](engine.md) — controlling an engine
- [Passive Scanner](passive_scanner.md) — scanning nearby objects
- [Resource Container](resource_container.md) — storing and moving resources
- [Asteroid Scanner](asteroid_scanner.md) — scanning an asteroid's composition
- [Asteroid Miner](asteroid_miner.md) — mining an asteroid into a container
- [Blueprints Library](blueprints_library.md) — reading blueprints and their cost
- [Shipyard](shipyard.md) — building a ship from a blueprint
- [Game](game.md) — following the match and its result

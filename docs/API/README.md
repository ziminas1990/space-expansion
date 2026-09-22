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

Each individual UDP frame can contain only the `Message` message described in
`Protocol.proto`.

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

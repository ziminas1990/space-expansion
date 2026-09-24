# Game — the IGame interface

Previous topic: [Shipyard](./shipyard.md)

Game is a virtual player module. Through it the client follows the match and
receives the result when the match ends.

## How to connect to Game

A module of type `"Game"` with the name `"Game"` is always connected to the
player's [commutator](./commutator.md), and there is exactly one. The module
implements the `IGame` interface, which has one command:

- `monitor` — receive the result when the match ends

## How a score is counted

The match scores each player by the resources stored in
[resource containers](./resource_container.md) on that player's ships. Only a
ship that also carries a [shipyard](./shipyard.md) is counted. Containers on a
ship without a shipyard add nothing. Every container on a counted ship is
added together.

The match sets a target mass for each resource that counts, and a target
score. For one resource, the player's share is the stored mass divided by
that target mass. A share is at most 1. The score is the target score
multiplied by the average of those shares.

The match ends when a player's score reaches the target score. That player
has stored the full target mass of every resource that counts.

## The monitor command

`monitor` subscribes the session to the match result. The value of the bool
is ignored. See [monitoring_concept.md](./monitoring_concept.md) to know more
about monitoring.

The server sends no reply while the match is still running. When the match
ends, it sends one `game_over_report` on this session.

If the match has already ended, the server sends the stored report at once.

Several sessions can monitor one Game module at the same time. Each of them
receives the same report. Closing a session stops updates for that session.
The other sessions keep receiving them.

For example:

```json
{
  "tunnelId": 1301,
  "timestamp": 0,
  "game": {
    "monitor": true
  }
}
```

When the match ends, the server sends:

```json
{
  "tunnelId": 1301,
  "timestamp": 900000000,
  "game": {
    "game_over_report": {
      "leaders": [
        { "player": "Buffet", "score": 1000 },
        { "player": "Trader", "score": 400 }
      ]
    }
  }
}
```

`leaders` lists every player, from the highest score to the lowest. The first
entry is the winner. Each entry has:

- `player` — the player's login
- `score` — that player's points

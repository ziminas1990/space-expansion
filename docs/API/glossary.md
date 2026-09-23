# Glossary

## Ingame time

Ingame time is the number of microseconds that have passed in the simulation
since the game world started. It begins at 0 when the server starts and
increases with each simulation tick.

The server writes the current ingame time into the `timestamp` field of every
`Message` it sends on an established session. The value is the ingame time at
which the message was formed. A `login` response is sent before a session
exists, so its `timestamp` stays unset.

Ingame time can flow faster or slower than real time, and its speed relative to
real time can change during the game. For example, if the server is overloaded,
ingame time can slow down relative to real time.

Because the world is advanced in [simulation ticks](#simulation-ticks), ingame
time is discrete.

See also: [System Clock](./system_clock.md)

## Simulation ticks

The game world is simulated in ticks, so time in the game world is discrete.
The length of a tick is chosen dynamically. The server caps one tick at
1 millisecond of ingame time. When the load is low, a tick can be only tens
of microseconds long.

A second of ingame time therefore contains at least 1000 ticks.

# ISessionControl interface — session control

Previous topic: [IAccessPanel](./access-panel.md)
Next topic: [ICommutator](./commutator.md)

First, what a session is.

After authorization the server gives the client a port. The client must send
all subsequent UDP messages to that port. The client controls many ships and
pieces of equipment, so a command needs an explicit address: which ship or
device it is for.

**Sessions** provide that address. A session is a virtual connection inside a
single UDP stream. Each session has its own unique numeric identifier, carried:

- in commands from the client, so the server can route the command to a
  particular device
- in replies from the server, so the client can match the reply to an earlier
  command and to a particular device

Creating these virtual connections is covered in the next chapter, on
commutators.

When a message is formed, whether a command or a reply, the session identifier
is placed in the `tunnelId` field of the top-level `Message`. The only messages
that leave this field unset are those of the `IAccessPanel` interface, already
covered in the previous chapter.

A session is a virtual connection to a particular device, so only messages for
that device may be sent on it. For example, a session opened to an engine
accepts only messages of the `IEngine` interface. Other messages are ignored.
Some devices may implement several interfaces at once.

Every session also implements the common `ISessionControl` interface, which
defines these messages:

- `close` — close the session
- `closed_ind` — notification that the session was closed
- `heartbeat` — a check that the connection is alive

The client sends `close` to close a session. Once the server has handled the
command, it closes the session and ignores further client messages that carry
that session's identifier.

The server sends `closed_ind` to tell the client that a session was closed. A
session may be closed by a `close` command or for another reason. Once
`closed_ind` has been received, the session is closed.

The server sends `heartbeat` itself when it has had no messages from the client
for 400 ms, and no more often than once every 400 ms. The server does not reply
to a `heartbeat` from the client. Any incoming message resets the timer. If
there are no messages for about 1.4 s, the server closes the UDP connection and
sends `closed_ind` on every open session.

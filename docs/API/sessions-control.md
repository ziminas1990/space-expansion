# ISessionControl interface — session control

Previous topic: [IAccessPanel](./access-panel.md)
Next topic: [ICommutator](./commutator.md)

## What is session?

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
that device may be sent on it. For example, a session opened to an RCS module
accepts only messages of the `IRCS` interface. Other messages are ignored.
Some devices may implement several interfaces at once.

Every session also implements the common `ISessionControl` interface, which
defines these messages:

- `close` — close the session
- `closed_ind` — notification that the session was closed

The client sends `close` to close a session. That is the only way for the
client to close a session, including one opened with `open_tunnel`. Once the
server has handled the command, it closes the session and ignores further
client messages that carry that session's identifier.

The server sends `closed_ind` to tell the client that a session was closed. A
session may be closed by a `close` command or for another reason. Once
`closed_ind` has been received, the session is closed.

## What is a root session?

A root session is a special session. Once the client logs in successfully, the
server allocates a new root session. The client receives that session's
identifier in the `access_granted` response.

A root session represents a UDP connection. It implements the `IRootSession`
interface, which defines these messages:

- `new_commutator_session` — open a new session to the player's commutator
- `commutator_session` — the response to a `new_commutator_session` command
- `heartbeat` — a check that the connection is still alive

If the server receives no messages from the client for 1600 ms, it closes the
root session and all of its child sessions. The player must log in again to
continue the game.

To prevent that, the server sends a `heartbeat` to the client after every
400 ms of silence. When the client receives `heartbeat`, it MUST send a
`heartbeat` response back as soon as possible. This is how the server uses the
root session to monitor the connection and keep it alive.

Sending `close` (`ISessionControl`) on the root session closes it and all of
its child sessions. That is equivalent to logging out.

When the client needs a session to the commutator, it sends a
`new_commutator_session` command. The server always creates a new session and
returns its identifier in a `commutator_session` response. The client then uses
that identifier to send messages to the commutator.

## Example

First, the client logs in:

```json
{
    "tunnelId": 0,
    "timestamp": 0,
    "accessPanel": {
        "login": "player",
        "password": "secret"
    }
}
```

The server responds with `access_granted`:

```json
{
    "tunnelId": 0,
    "timestamp": 0,
    "accessPanel": {
        "access_granted": {
            "port": 25042,
            "session_id": 224
        }
    }
}
```

The client then sends this command on the root session to open a new session
to the commutator:

```json
{
    "tunnelId": 224,  // the root session id
    "timestamp": 0,
    "rootSession": {
        "new_commutator_session": true  // the value does not matter
    }
}
```

The server responds with `commutator_session`:

```json
{
    "tunnelId": 224,  // the root session id
    "timestamp": 0,
    "rootSession": {
        "commutator_session": 456  // the new session id
    }
}
```

The client can now use session identifier 456 to send messages to the
commutator:

```json
{
    "tunnelId": 456,  // the commutator session id
    "timestamp": 0,
    "commutator": {
        "total_slots_req": true
    }
}
```

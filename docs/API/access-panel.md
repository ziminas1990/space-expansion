# IAccessPanel interface — connecting to the server

Previous topic: [README](./README.md)
Next topic: [Sessions Management](./sessions-management.md)

Messages from the **IAccessPanel** interface are used to connect to the server.

The interface has only one command, `login`, with which the client must send:

- login — the user name
- password — the user password

To send a message to the server, wrap it in a `Message`. `Message` is the
top-level protocol message. It is a container for every other message (via a
oneof), and it also has several important fields:

- tunnelId — the session (tunnel) identifier
- timestamp — an ingame time mark

`tunnelId` will be covered in more detail later. The meaning of `timestamp`
depends on the direction of the message. If the message goes from the client to
the server (a command), `timestamp` determines the ingame time at which the
command must be executed. If the message goes from the server to the client (a
response), `timestamp` determines the ingame time at which the response was
produced.

In a `login` message the server ignores `tunnelId` and `timestamp`, so they can
be left unset.

In protobuf, a zero value of a scalar field is equivalent to the field being
absent. A field with the value 0 is not transmitted, and a missing field is read
as 0 when the message is parsed. That is why these fields are 0 in every example
below.

The `login` message therefore has the following structure:

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

`Message` must be sent to the server as a UDP datagram. That requires the server
IP and port. If you run the server on your own machine, use **127.0.0.1** as the
IP and the default port **6842**. If you connect to an external server, ask the
server administrator for its IP and port.

If `login` contained a wrong login/password pair, or no such user exists, the
server sends an `access_rejected` message with the string
`Invalid login or password`. The response has the following structure:

```json
{
    "tunnelId": 0,
    "timestamp": 0,
    "accessPanel": {
        "access_rejected": "Invalid login or password"
    }
}
```

If authorization succeeds, the server sends an `access_granted` message, which
contains:

- `port` — the UDP port to which the client must send all subsequent messages
- `session_id` — the identifier of the root session (the `IRootSession`
  interface)

Here is an example of such a message:

```json
{
    "tunnelId": 0,
    "timestamp": 0,
    "accessPanel": {
        "access_granted": {
            "port": 25042,
            "session_id": 1234567890
        }
    }
}
```

In this example the server issued port 25042. The client must send all
subsequent messages to the same server IP and to this port.

On the issued port the server accepts messages only from the IP and port from
which the `login` command was received. Messages from a different source address
are ignored.

One player can have at most 16 such sessions at the same time. If the limit is
already reached, the server replies with `access_rejected` and the string
`Connections limit reached`.

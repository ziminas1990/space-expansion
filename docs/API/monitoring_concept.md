# A common approach to implementing monitoring

Most commands in the `Protocol.proto` protocol work in request-response mode.
When the client sends a command on a session, it expects a final response
within a finite time, and handling of that command ends there. The next
command can then be sent on the session.

For example, the `ICommutator.total_slots_req` command means the server sends
exactly one `ICommutator.total_slots` message, containing the total number of
slots in the commutator, and sends no further messages.

Many modules, however, assume that their state can change over time or as a
result of handling commands. Two general approaches exist for tracking such
changes on the client in good time:

- **polling model** — the client periodically requests the module's state itself
- **push model** — the server sends updates to the client itself

Polling is the simpler and more straightforward approach, but it requires the
client to send regular requests to the server and is unsuitable when the
client must react to changes promptly.

The push model solves the problems of the polling model, but it is harder to
implement.

`Protocol.proto` uses a common approach for the push model. Some modules
support this mode through the `monitor` command.

If `monitor` is sent on a session, the server remembers that this session
should receive updates for the corresponding module. Updates then go to every
open session on which `monitor` was sent for that module. A module may send
them when events occur, or regularly, as a subscription with some interval.

The stream of messages that follows `monitor` is entirely determined by the
specific module and is described in that module's documentation. This stream
has no structure common to all modules.

After `monitor` is sent, the session can still be used for other commands, but
the best practice is to leave it only for receiving updates. The `close`
command of the `ISessionControl` interface closes the session, and the server
stops sending notifications on it.

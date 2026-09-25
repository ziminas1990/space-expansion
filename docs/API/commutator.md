# ICommutator interface — connecting to devices

Previous topic: [ISessionControl](./sessions-control.md)
Next topic: [Ship](./ship.md)

A commutator is a device to which other devices can be connected, both physical
ones, such as RCS thrusters or an onboard scanner, and virtual ones, such as the
system clock or a blueprints library.

The client talks to a commutator through the `ICommutator` interface. These
commands are available:

- `total_slots_req` — request the total number of slots in the commutator
- `module_info_req` — request information about the module installed in a slot
- `all_modules_info_req` — request information about every module installed in
  the commutator
- `open_tunnel` — open a new session to a module
- `monitor` — start a session that monitors the state of the commutator

Each command is covered below.

## What is a slot?

A commutator slot is the point where a device connects to the commutator. Slots
are numbered from 0 to N-1, where N is the number of slots in the commutator.

To learn the total number of slots, the client sends `total_slots_req`. The
server replies with a `total_slots` message that contains that number.

To learn about the module installed in a slot, the client sends
`module_info_req`. The server replies with a `module_info` message that
contains these fields:

- `slot_id` — the slot identifier (the same number as in the request)
- `module_type` — the module type, or `"empty"` if the slot is empty or does
  not exist
- `module_name` — the module name
- `blueprint_name` — the name of the blueprint the module was created from

`module_type` is a string literal drawn from a fixed set of values. It
determines which interfaces the module implements. For example, a module of
type `"RCS"` is the hull thrusters and implements `IRCS`. The full list of
module types and their interfaces is in the [module table](./modules_table.md).

`module_name` is an arbitrary name of the module within the commutator. It is
guaranteed to be unique for every module in the commutator.

`blueprint_name` is the name of the blueprint the module was created from.
Blueprints are covered in [Blueprints Library](./blueprints_library.md).

If the requested slot is empty, or no such slot exists (the number is greater
than or equal to the number of slots), the server still replies with a single
`module_info` and does not send an error code. In that reply `module_type` is
`"empty"`, and `module_name` and `blueprint_name` are empty. `slot_id` repeats
the number from the request.

## The all_modules_info_req command

To get information about every module installed in the commutator at once, the
client sends `all_modules_info_req`. The server replies with a single
`modules_info_list` message. That list has one entry for each occupied slot.
Empty slots are left out. Each entry has the same fields as `module_info`.

If the commutator has no modules, the server still sends one
`modules_info_list`, and that list is empty.

## How to connect to a module

To connect to a module, the client sends `open_tunnel` with the identifier of
the slot in which the module is installed.

If the connection is established, the server sends `open_tunnel_report`, which
carries the identifier of the new session used to talk to the module. Every
message sent on that session is delivered to that module for handling.

If the connection cannot be established, the server sends `open_tunnel_failed`
with one of these codes:

| Code                  | Description                                          |
| --------------------- | ---------------------------------------------------- |
| INVALID_SLOT          | The specified slot does not exist                    |
| MODULE_OFFLINE        | The slot is empty or the module is offline           |
| REJECTED_BY_MODULE    | The module's session limit is reached                |
| COMMUTATOR_OFFLINE    | The commutator is unavailable                        |

The only way to close that session is `close` on the session itself
(`ISessionControl`). The server then sends `closed_ind` on the closed session
(see [sessions-control.md](./sessions-control.md)).

## How to monitor a commutator

While a commutator is running, new modules can be attached to it and existing
ones can be detached. The `monitor` command of `ICommutator` lets the client
learn about these changes as they happen.

The `monitor` command implements the push model described in
[monitoring_concept.md](./monitoring_concept.md).

The server immediately replies with `monitor_ack` and one of these codes:

| Code                  | Description                                          |
| --------------------- | ---------------------------------------------------- |
| SUCCESS               | Subscription accepted; updates will follow           |
| TOO_MANY_SESSIONS     | 8 monitoring sessions are already open; rejected     |

While the session stays open, the server sends `update` messages when the set
of modules changes:

- `module_attached` — a module was attached to a slot. The fields are the same
  as in `module_info`: `slot_id`, `module_type`, `module_name`,
  `blueprint_name`.
- `module_detached` — a module was detached from a slot. The message contains
  the number of that slot.

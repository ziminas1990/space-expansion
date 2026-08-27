# TypeScript SDK architecture

## Sources of truth

- The wire contract is `../server/Protocol.proto`.
- `../python-sdk/` is a behavioral reference, not a layering reference.
- Never edit generated `*_pb.ts` files manually.

## Layer boundaries

### Transport

`transport/` moves bytes and protobuf messages, queues incoming messages, and
routes sessions. It contains no game API or domain behavior.

### Lowlevel

- One lowlevel call corresponds to exactly one protocol message: one send or one
  receive.
- A class wraps one interface from `Protocol.proto`.
- Lowlevel validates and converts that message, but does not manage session
  pools, combine exchanges, cache results, or model world entities.

### Midlevel

- A class still corresponds to exactly one interface from `Protocol.proto`.
- It owns the session pool and combines one or more lowlevel calls into a logical
  server operation (send request and wait for a reply).
- Every public operation performs at least one server exchange. Midlevel must not
  return cached RPC results.
- Pagination and request/report/terminal-status loops belong here. Long-running
  operations use a dedicated session.
- Different interfaces use different midlevel clients and pools, even when they
  share the same `open_session_cb`.

### Highlevel

- Highlevel models user-facing entities and workflows rather than protocol
  interfaces.
- It may cache state, predict values, run background monitoring, expose events,
  and coordinate complex procedures.
- One highlevel class may compose multiple midlevel interfaces.
- Example: `highlevel.Ship` combines `midlevel.Ship` (`IShip`),
  `midlevel.Navigation` (`INavigation`), and `midlevel.Commutator`
  (`ICommutator`) using the same ship-slot `open_session_cb`.
- Highlevel owns the lifecycle of all clients it composes and terminates them
  together.

## Design rule

Do not copy Python classes across layers mechanically. Decide whether a concept
is a protocol interface, a composed server operation, or a world entity, then
place it in lowlevel, midlevel, or highlevel respectively.

## Build

Run `npm run build` from `typescript-sdk/`. It regenerates protobuf sources and
runs TypeScript compilation.

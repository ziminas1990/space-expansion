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
- `monitoring()` is a dedicated-session loop. The callback is
  `(value: T | undefined) => Promise<boolean>` and does not take `Status`: a
  payload is passed on a real update; a failed wait returns that status from
  `monitoring()` itself. A wait timeout is not a failure — the loop calls the
  callback with `undefined` so upper logic can check a stop flag and return
  `false` to leave the loop (`Status.ok()`), or `true` to keep waiting.
- Different interfaces use different midlevel clients and pools, even when they
  share the same `open_session_cb`.

### Highlevel

- Highlevel models user-facing entities and workflows rather than protocol
  interfaces.
- It may cache state, predict values, run background monitoring, expose events,
  and coordinate complex procedures.
- One highlevel class may compose multiple midlevel interfaces.
- Example: `highlevel.Ship` wraps `midlevel.Ship` (`IShip`) and takes nested
  `INavigation` / `ICommutator` clients from `midlevel.Ship.navigator()` /
  `commutator()`. Highlevel does not open those tunnels itself.
- Highlevel does not import `lowlevel/` and does not reimplement protocol loops.
- Highlevel does not open sessions or call `session.close()`. The session pool
  belongs to `midlevel.BaseModule`. `open_session_cb` is a midlevel
  implementation detail; it does not appear in public highlevel signatures.
  Hidden `ModuleRegistry` reads it only inside `spawn_client`.

There is no highlevel base **class**. Slot wrappers implement the `BaseModule`
interface (`type`, `name`, `release`, `reinit`). `Navigation` and `Game` do
not implement it (no slot, no `reinit`). Narrowing to a concrete type is
`module.type === ModuleType.ENGINE` on the `HighlevelModule` union, not
`instanceof`.

Each wrapper follows this pattern:

- Ordinary class, `implements BaseModule` on all 10 slot modules;
  `extends EventEmitter<Events>` only when the wrapper publishes events.
- `readonly type` with a literal (`readonly type = ModuleType.ENGINE`) is the
  discriminant of `HighlevelModule`.
- `constructor(private rpc: midlevel.X, readonly name: string)` — omit `name`
  for objects not bound to a slot (`Navigation`, `Game`).
- `rpc` stays private. 1:1 wrappers expose it with argument-free
  `down_level(): M`. Aggregates that own several midlevel clients (`Ship`,
  `Player`) take a discriminator and return the matching client, not a
  highlevel wrapper.
- `release()` is the only highlevel teardown. There is no highlevel
  `terminate()`. Internally `release()` calls `rpc.terminate()` **first**
  (sessions close, waiters wake), then awaits own background tasks and resets
  caches.

## Design rule

Do not copy Python classes across layers mechanically. Decide whether a concept
is a protocol interface, a composed server operation, or a world entity, then
place it in lowlevel, midlevel, or highlevel respectively.

## Package

The installable package is `@spx/sdk`. Consumers import the public API through
package specifiers such as `@spx/sdk/highlevel` and `@spx/sdk/highlevel/ship`.
Do not import SDK sources by relative path from applications.

Inside the library, cross-layer imports use Node subpath imports from
`package.json` (`#sdk/midlevel/…`, `#sdk/types/…`, `#sdk/Protocol_pb.js`).
Same-folder imports stay relative (`./ship.js`). Applications never use `#…`
specifiers.

The harvester is a separate private application in `demo/harvester/`. It
depends on `@spx/sdk` and is not part of the library build. Start it with
`npm run harvester` from this directory.

## Build

Run `npm run build` from `typescript-sdk/`. It regenerates protobuf sources and
runs TypeScript compilation. The library compile does not include tests or
demos.

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
- Example: `highlevel.Ship` wraps `midlevel.Ship` (`IShip`) and takes nested
  `INavigation` / `ICommutator` clients from `midlevel.Ship.navigator()` /
  `commutator()`. Highlevel does not open those tunnels itself.
- Highlevel does not import `lowlevel/` and does not reimplement protocol loops.
- **Highlevel does not open or close sessions.** The session pool belongs
  entirely to `midlevel.BaseModule`. `open_session_cb` is a midlevel
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
- Caches are ordinary `Cached<T>` fields, or a `Map` when keyed by id. There is
  no central cache registry.
- `init(): Promise<Status>` exists only on wrappers that run background work
  the registry must start.
- Every wrapper has `release(): Promise<Status>`: set a stop flag, await own
  background tasks, reset own caches. It does **not** call `rpc.terminate()`.
- Hidden `ModuleRegistry` (not exported from `highlevel/index.ts`) **owns**
  midlevel slot clients: it creates them with `midlevel.create_module` and
  the slot's `open_session_cb`, stores them in `clients`, and calls
  `terminate()` on detach and in `release()`. After `terminate()`,
  `midlevel.BaseModule` immediately returns `Status.closed("terminated")` on
  any attempt to open a session. Highlevel does not keep an `alive` flag
  and does not fail-fast with its own status; public methods just forward the
  midlevel error.
- The wrapper instance stays in the user's hands across detach. The
  registry keeps it in `known` and calls `reinit(new_rpc)` on reattach of
  the same `(type, name)` instead of `new`. Nested nav/commutator clients of
  a ship die with `midlevel.Ship.terminate()`, which the parent registry
  calls on ship detach. `highlevel.Ship.release()` must not terminate its
  own `midlevel.Ship`.
- Midlevel `Commutator` does not track attached modules. `get_all_modules_info`
  and `monitoring` return snapshots and updates only. Opening a tunnel stays on
  midlevel; remembering which slots are live is highlevel (`ModuleRegistry`).
- There are no static `find()` selectors. Pick a module with `get_all` /
  `get_by_name` on `Ship` or `Player` (delegated to the hidden registry).

## Design rule

Do not copy Python classes across layers mechanically. Decide whether a concept
is a protocol interface, a composed server operation, or a world entity, then
place it in lowlevel, midlevel, or highlevel respectively.

## Build

Run `npm run build` from `typescript-sdk/`. It regenerates protobuf sources and
runs TypeScript compilation.

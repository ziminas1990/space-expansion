# TypeScript SDK development guide

## Sources of truth

- The wire API is defined by `../server/Protocol.proto`.
- The complete reference implementation is `../python-sdk/expansion/`.
- Generated `*_pb.ts` files come from the server proto files. Never edit them manually.
- Before implementing an interface, compare its proto definition with the matching Python RPC and module implementations.

## Architecture

The SDK deliberately has four layers:

1. `transport/`: UDP transport, protobuf encoding, channel/terminal abstractions, and incoming-message queues.
2. `lowlevel/`: thin, one-to-one wrappers around protobuf interfaces plus session routing.
3. `midlevel/`: composed RPC operations, retries, tunnel creation, and reusable session pools.
4. `highlevel/`: user-facing objects, cached state, monitoring, events, and `Player`.

Python-to-TypeScript mapping:

- `expansion/transport/` → `transport/` and low-level session classes.
- `expansion/interfaces/rpc/` → `lowlevel/`.
- `expansion/modules/` → `midlevel/`.
- `expansion/procedures/` plus user-facing composition → `highlevel/`.

The root `index.ts` exports high-level APIs directly and exposes `midlevel`, `lowlevel`, `transport`, and generated protobuf messages as namespaces.

## Message and session flow

The normal stack is:

`UdpSocket → MessagesDecoder → RootSession → lowlevel interface → midlevel module → highlevel facade`

- `Session.send()` stamps every message with its `tunnelId`.
- `Session` handles heartbeat and close control messages without forwarding them.
- `RootSession` routes messages to child sessions by `tunnelId`.
- Opening a module tunnel returns a server session ID, which must be registered with `RootSession`.
- Login first talks to `AccessPanel` on UDP port 6842, reconnects to the granted port, and replaces it with `RootSession`.

## Error-handling conventions

- Expected failures are represented by `Status`, not exceptions.
- Operations returning data normally use `Promise<[Status, T | undefined]>`.
- Operations without data normally use `Promise<Status>`.
- Add context while propagating failures with `status.wrap("context")`.
- Check both `status.is_ok()` and the returned optional value.
- Timeouts are milliseconds. Do not confuse IDs or slot numbers with timeout arguments.
- `Status` is delivery/protocol only (`ok`, timeout, closed, unexpected message shape).
- Protobuf interface enums (`SUCCESS`, `SCANNER_BUSY`, `ROUTED`, …) stay as a separate typed value next to `Status`. A delivered server failure is still `Status.ok()` plus that enum.
- Do not fold a server enum into `Status.fail()` in new lowlevel code.

## Time and numeric precision

- Proto `uint64` time fields (`Message.timestamp`, clock `time`/`ring`/`wait_until`/`wait_for`) are `bigint`. Do not convert them with `Number()`.
- Use `ServerTimestamp { real_us: bigint; ingame_us: bigint }` for clock payloads and `asUint64()` when a value might arrive as `number`.
- Existing `ShipState.timestamp` stays `number` so midlevel/highlevel stay unchanged; new interfaces must not copy that conversion.

## Low-level implementation pattern

- Build protobuf values with `create(...Schema, {...})` from `@bufbuild/protobuf`.
- Handle generated `oneof` fields through `{ case, value }`.
- Keep the layer protocol-oriented: no caching, module registry, or business workflow.
- Follow the existing explicit `send_*_request()` and `wait_*_response()` split.
- Validate the outer `Message.choice.case` and the interface-level response case.
- Convert protobuf values into small SDK domain types before returning them.
- Reuse `types/` converters for `Position`, `PhysicalObject`, resources, and blueprints; include `RESOURCE_STONE`.
- Use ESM imports with `.js` extensions, even in TypeScript source.

## Mid-level implementation pattern

- Extend `BaseModule<LowlevelInterface>`.
- Accept an `OpenSessionCallback` and construct the low-level wrapper for each acquired session.
- Public methods call `run()` or `run_no_return()` and combine the low-level send/wait pair.
- Reusable request sessions return to the pool after success.
- Monitoring owns a dedicated session and therefore calls `run(..., true)`.
- Tunnel-opening methods register the reported session ID with `RootSession`.
- Enrich discovered module information with an `open_session_cb` bound to its slot.

## High-level implementation pattern

- Expose user-oriented state and behavior rather than protobuf details.
- Start long-running mid-level monitoring in the background.
- Maintain local collections/state and emit attach, detach, reset, or update events.
- Stop and release monitoring when a module disappears.
- Server ship types use the `Ship/` prefix; use the same convention for attachment and detachment.
- `Player` is a TypeScript-specific facade and has no direct Python equivalent.

## Adding a module

1. Find the interface in `Protocol.proto` and its complete Python implementation.
2. Add and export `lowlevel/<module>.ts`.
3. Add and export `midlevel/<module>.ts` using `BaseModule`.
4. Add a high-level facade only when cached state, events, or lifecycle management are useful.
5. Register module-type dispatch in the high-level composition/factory.
6. Build the package and test request, timeout, monitoring, and session-close paths.

## Build

- Work from `typescript-sdk/` (`cd` there; the repo root has no `package.json`).
- Install dependencies with `npm install`. `protoc` is a local devDependency; `npm run regenerate` must call that binary, not `npx protoc`.
- Run `npm run build`; it regenerates protobuf TypeScript and then runs `tsc`.
- Do not commit or hand-maintain generated protobuf output unless repository policy changes.
- If a command hangs on `npx` or missing `protoc`, stop it and rerun `npm run build` with unrestricted permissions. Do not wait on `npx protoc`.

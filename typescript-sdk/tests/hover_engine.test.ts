import { expect, test } from "vitest";
import type { Ship, ShipState } from "../highlevel/index.js";
import { HoverEngine, ModuleType } from "../midlevel/index.js";
import { Status } from "../types/status.js";
import type { Vector } from "../types/common.js";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    General,
    makeTug,
    Player,
    Position,
    World,
} from "./configurator/index.js";
import {
    hasServerBinary,
    integrationTimeoutMs,
    withServer,
} from "./fixture.js";
import { IngameClock } from "./ingame_clock.js";
import {
    Collector,
    expectOk,
    expectStatus,
    getShip,
} from "./helpers/index.js";

const MAX_THRUST = 10_000;
const SHIP_MASS = 200_000;
const LONG_BURN_MS = 100_000;
const PROCEED_TIMEOUT_MS = 15_000;

function engineConfiguration(): Configuration {
    return new Configuration({
        general: new General({
            totalThreads: 1,
            loginUdpPort: 7_456,
            initialState: ApplicationMode.Freeze,
            portsPool: [12_000, 12_100],
        }),
        blueprints: new DefaultBlueprints(),
        world: new World(),
        players: [
            new Player({
                login: "player",
                password: "player",
                ships: [makeTug("tug-1", new Position(0, 0))],
            }),
        ],
    });
}

// Highlevel keeps one state object and updates it in place. Copy it before the
// next tick, or a later sample overwrites the earlier one.
function snapshot(state: ShipState): ShipState {
    return {
        timestamp: state.timestamp,
        weight: state.weight,
        orientation: [state.orientation[0], state.orientation[1]],
        position: {
            timestamp: state.position.timestamp,
            point: [state.position.point[0], state.position.point[1]],
            velocity: [state.position.velocity[0], state.position.velocity[1]],
        },
    };
}

function noseOf(orientation: Vector): Vector {
    const length = Math.hypot(orientation[0], orientation[1]);
    return [orientation[0] / length, orientation[1] / length];
}

function noseAngle(orientation: Vector): number {
    return Math.atan2(orientation[1], orientation[0]);
}

function expectNose(orientation: Vector, expected: number): void {
    expect(Math.hypot(orientation[0], orientation[1])).toBeCloseTo(1, 2);
    let delta = noseAngle(orientation) - expected;
    if (delta > Math.PI) {
        delta -= 2 * Math.PI;
    } else if (delta < -Math.PI) {
        delta += 2 * Math.PI;
    }
    expect(Math.abs(delta)).toBeLessThan(0.05);
}

function expectAcceleration(before: ShipState, after: ShipState, thrust: number): void {
    const mass = before.weight;
    expect(mass).toBe(SHIP_MASS);
    const dt = (after.timestamp - before.timestamp) / 1_000_000;
    expect(dt).toBeGreaterThan(0.5);
    const nose = noseOf(before.orientation);
    const accel = thrust / mass!;
    expect(after.position.velocity[0]).toBeCloseTo(
        before.position.velocity[0] + nose[0] * accel * dt,
        3,
    );
    expect(after.position.velocity[1]).toBeCloseTo(
        before.position.velocity[1] + nose[1] * accel * dt,
        3,
    );
}

function expectCoasting(before: ShipState, after: ShipState): void {
    const dt = (after.timestamp - before.timestamp) / 1_000_000;
    expect(dt).toBeGreaterThan(0.5);
    expect(after.position.velocity[0]).toBeCloseTo(before.position.velocity[0], 4);
    expect(after.position.velocity[1]).toBeCloseTo(before.position.velocity[1], 4);
}

// While the ingame clock is stopped, the server applies a request on the next
// tick. Step time until that request settles.
async function duringTicks<T>(
    clock: IngameClock,
    request: Promise<[Status, T | undefined]>,
    description: string,
): Promise<T> {
    let settled: [Status, T | undefined] | undefined;
    const pending = request.then((value) => {
        settled = value;
    });
    const deadline = Date.now() + 3_000;
    while (settled === undefined && Date.now() < deadline) {
        await clock.proceed(50, 2_000);
        await Promise.race([
            pending,
            new Promise((resolve) => setTimeout(resolve, 20)),
        ]);
    }
    await pending;
    if (settled === undefined) {
        throw new Error(`${description}: no response`);
    }
    return expectOk(settled, description);
}

async function openHoverEngine(ship: Ship): Promise<HoverEngine> {
    const modules = expectOk(
        await ship.down_level("commutator").get_all_modules_info(),
        "list ship modules",
    );
    const info = modules.find((item) => item.module_type === ModuleType.HOVER_ENGINE);
    if (info === undefined) {
        throw new Error("HoverEngine not found");
    }
    expect(info.module_name).toBe("engine");
    return new HoverEngine(info.open_session_cb);
}

async function waitApplied(
    engine: HoverEngine,
    clock: IngameClock,
    expected: number,
): Promise<void> {
    const deadline = Date.now() + 3_000;
    let applied = -1;
    while (Date.now() < deadline) {
        applied = await duringTicks(clock, engine.get_thrust(), "applied thrust");
        if (applied === expected) {
            return;
        }
    }
    expect(applied).toBe(expected);
}

async function turnTo(
    ship: Ship,
    clock: IngameClock,
    x: number,
    y: number,
): Promise<void> {
    const spec = await duringTicks(
        clock,
        ship.get_specification(),
        "ship specification",
    );
    const speed = spec.max_rotation_speed;
    const state = snapshot(await duringTicks(clock, ship.get_state(0), "nose before turn"));
    const from = noseOf(state.orientation);
    const to = noseOf([x, y]);
    const angle = Math.abs(Math.atan2(
        from[0] * to[1] - from[1] * to[0],
        from[0] * to[0] + from[1] * to[1],
    ));
    const durationMs = (angle / speed) * 1_000;

    const [status] = await ship.down_level("ship").run(async (session) => {
        expectStatus(await session.send_rotate(x, y, speed), "send rotate");
        const ack = session.wait_rotate_ack(PROCEED_TIMEOUT_MS);
        await clock.proceed(durationMs + 100, PROCEED_TIMEOUT_MS);
        expectStatus(await ack, "rotate ack");
        return [Status.ok(), undefined];
    });
    expectStatus(status, "rotate");
}

async function waitJournal(
    clock: IngameClock,
    journal: Collector<number>,
    count: number,
    description: string,
): Promise<void> {
    const deadline = Date.now() + 2_000;
    while (journal.length < count && Date.now() < deadline) {
        await clock.proceed(50, 2_000);
        await new Promise((resolve) => setTimeout(resolve, 20));
    }
    await journal.waitForCount(count, description);
}

async function monitorThrust(
    engine: HoverEngine,
    journal: Collector<number>,
    stopped: { value: boolean },
): Promise<void> {
    expectStatus(
        await engine.monitoring(async (thrust) => {
            if (thrust !== undefined) {
                journal.push(thrust);
            }
            return !stopped.value;
        }),
        "hover engine monitoring",
    );
}

test.skipIf(!hasServerBinary)(
    "thrusts along the nose and never above the maximum",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(engineConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins and opens the hover engine
            const player = await login("player", "player");
            const ship = getShip(player, "tug-1");
            const engine = await openHoverEngine(ship);

            try {
                // 2. the specification limit is the maximum thrust, and the nose is +X
                const spec = expectOk(
                    await engine.get_specification(),
                    "hover engine specification",
                );
                expect(spec.max_thrust).toBe(MAX_THRUST);
                const initial = expectOk(await ship.get_state(0), "initial state");
                expect(initial.weight).toBe(SHIP_MASS);
                expectNose(initial.orientation, 0);
                await clock.stop();

                // 3. a request above the maximum is applied at the maximum
                expectStatus(
                    await engine.set_thrust(spec.max_thrust * 2, LONG_BURN_MS),
                    "set thrust above max",
                );
                await waitApplied(engine, clock, spec.max_thrust);

                // 4. the ship accelerates along its nose at that thrust over mass
                const burning = snapshot(await duringTicks(
                    clock,
                    ship.get_state(0),
                    "state while burning",
                ));
                await clock.proceed(1_000, PROCEED_TIMEOUT_MS);
                const moved = snapshot(await duringTicks(
                    clock,
                    ship.get_state(0),
                    "state after the burn",
                ));
                expectAcceleration(burning, moved, spec.max_thrust);

                // 5. setting the thrust to zero stops that acceleration
                expectStatus(await engine.set_thrust(0, 0), "set thrust to zero");
                await waitApplied(engine, clock, 0);
                const coast = snapshot(await duringTicks(
                    clock,
                    ship.get_state(0),
                    "state at the start of the coast",
                ));
                await clock.proceed(1_000, PROCEED_TIMEOUT_MS);
                const coasted = snapshot(await duringTicks(
                    clock,
                    ship.get_state(0),
                    "state after coasting",
                ));
                expectCoasting(coast, coasted);

                // 6. turn the nose to +Y while the engine is thrusting again
                expectStatus(
                    await engine.set_thrust(spec.max_thrust, LONG_BURN_MS),
                    "set thrust before the turn",
                );
                await waitApplied(engine, clock, spec.max_thrust);
                await turnTo(ship, clock, 0, 1);
                const turned = snapshot(await duringTicks(
                    clock,
                    ship.get_state(0),
                    "state after the turn",
                ));
                expectNose(turned.orientation, Math.PI / 2);
                expect(await duringTicks(clock, engine.get_thrust(), "thrust after the turn"))
                    .toBe(spec.max_thrust);

                // 7. further acceleration follows the new nose at the same magnitude
                await clock.proceed(1_000, PROCEED_TIMEOUT_MS);
                const afterTurn = snapshot(await duringTicks(
                    clock,
                    ship.get_state(0),
                    "state after thrusting along the new nose",
                ));
                expectAcceleration(turned, afterTurn, spec.max_thrust);
            } finally {
                await engine.terminate();
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "monitors thrust changes and ignores a turn",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(engineConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins and opens the hover engine
            const player = await login("player", "player");
            const ship = getShip(player, "tug-1");
            const engine = await openHoverEngine(ship);
            const spec = expectOk(
                await engine.get_specification(),
                "hover engine specification",
            );
            await clock.stop();

            // 2. start monitoring
            const journal = new Collector<number>();
            const stopped = { value: false };
            const monitoring = monitorThrust(engine, journal, stopped);

            try {
                // 3. the snapshot is zero thrust
                await waitJournal(clock, journal, 1, "thrust snapshot");
                expect(journal.items[0]).toBe(0);

                // 4. an applied thrust is reported, and the same command is reported again
                expectStatus(await engine.set_thrust(100, LONG_BURN_MS), "set thrust");
                await waitJournal(clock, journal, 2, "applied thrust");
                expect(journal.items[1]).toBe(100);
                expectStatus(
                    await engine.set_thrust(100, LONG_BURN_MS),
                    "repeat the same thrust",
                );
                await waitJournal(clock, journal, 3, "repeated thrust");
                expect(journal.items[2]).toBe(100);

                // 5. a thrust above the maximum is reported at the maximum
                expectStatus(
                    await engine.set_thrust(spec.max_thrust * 2, LONG_BURN_MS),
                    "set thrust above max",
                );
                await waitJournal(clock, journal, 4, "clamped thrust");
                expect(journal.items[3]).toBe(spec.max_thrust);

                // 6. turning the ship does not change the reported magnitude
                const beforeTurn = journal.length;
                await turnTo(ship, clock, 0, 1);
                await new Promise((resolve) => setTimeout(resolve, 150));
                expect(journal.length).toBe(beforeTurn);
                expect(await duringTicks(clock, engine.get_thrust(), "thrust after the turn"))
                    .toBe(spec.max_thrust);

                // 7. when the duration elapses, the monitor receives zero
                expectStatus(await engine.set_thrust(80, 300), "start a short burn");
                await waitJournal(clock, journal, beforeTurn + 1, "short burn applied");
                expect(journal.items[beforeTurn]).toBe(80);
                await waitJournal(clock, journal, beforeTurn + 2, "short burn expired");
                expect(journal.items[beforeTurn + 1]).toBe(0);
            } finally {
                stopped.value = true;
                await monitoring;
                await engine.terminate();
            }
        });
    },
);

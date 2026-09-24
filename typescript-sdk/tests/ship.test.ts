import { expect, test } from "vitest";
import type { Vector as Nose } from "../types/common.js";
import { Status } from "../types/status.js";
import type { IngameClock } from "./ingame_clock.js";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    General,
    makeMiner,
    makeProbe,
    Player,
    Position,
    ShipBlueprint,
    Vector,
    World,
} from "./configurator/index.js";
import {
    hasServerBinary,
    integrationTimeoutMs,
    withServer,
} from "./fixture.js";
import {
    expectOk,
    expectStatus,
    getBlueprintsLibrary,
    getShip,
} from "./helpers/index.js";

const PROCEED_TIMEOUT_MS = 15_000;
const TICK_US = 1_000;
const ANGLE_TOLERANCE = 0.08;

function shipConfiguration(): Configuration {
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
                ships: [
                    makeProbe("plain", new Position(0, 0)),
                    makeMiner("nosed", new Position(100, 0))
                        .setOrientation(new Vector(0, 2)),
                ],
            }),
        ],
    });
}

function shipBlueprint(name: string): ShipBlueprint {
    const blueprint = new DefaultBlueprints().blueprints.get(`Ship/${name}`);
    if (!(blueprint instanceof ShipBlueprint)) {
        throw new Error(`Ship blueprint '${name}' is not configured`);
    }
    return blueprint;
}

function noseAngle(orientation: Nose): number {
    return Math.atan2(orientation[1], orientation[0]);
}

function angleDelta(actual: number, expected: number): number {
    let delta = actual - expected;
    if (delta > Math.PI) {
        delta -= 2 * Math.PI;
    } else if (delta < -Math.PI) {
        delta += 2 * Math.PI;
    }
    return delta;
}

function expectNose(orientation: Nose, expected: number, tolerance = ANGLE_TOLERANCE): void {
    expect(Math.hypot(orientation[0], orientation[1])).toBeCloseTo(1, 2);
    expect(Math.abs(angleDelta(noseAngle(orientation), expected))).toBeLessThan(tolerance);
}

async function advance(clock: IngameClock, ms: number): Promise<void> {
    await clock.proceed(ms, PROCEED_TIMEOUT_MS, TICK_US);
}

test.skipIf(!hasServerBinary)(
    "reports the blueprint rotation limit and the current nose",
    { timeout: integrationTimeoutMs },
    async () => {
        const probe = shipBlueprint("Probe");
        const miner = shipBlueprint("Miner");

        await withServer(shipConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("player", "player");
            const plain = getShip(player, "plain");
            const nosed = getShip(player, "nosed");

            // 2. each ship reports the rotation speed and the radius set on its blueprint
            const plainSpec = expectOk(
                await plain.get_specification(),
                "plain specification",
            );
            const nosedSpec = expectOk(
                await nosed.get_specification(),
                "nosed specification",
            );
            expect(plainSpec.max_rotation_speed).toBeCloseTo(probe.max_rotation_speed, 6);
            expect(nosedSpec.max_rotation_speed).toBeCloseTo(miner.max_rotation_speed, 6);
            expect(plainSpec.max_rotation_speed).not.toBe(nosedSpec.max_rotation_speed);
            expect(plainSpec.radius).toBeCloseTo(probe.radius, 6);
            expect(nosedSpec.radius).toBeCloseTo(miner.radius, 6);
            expect(plainSpec.radius).not.toBe(nosedSpec.radius);

            // 3. the published blueprint carries that same speed
            const library = getBlueprintsLibrary(player);
            const plainBlueprint = expectOk(
                await library.get_blueprint("Ship/Probe"),
                "probe blueprint",
            );
            const nosedBlueprint = expectOk(
                await library.get_blueprint("Ship/Miner"),
                "miner blueprint",
            );
            expect(publishedSpeed(plainBlueprint.properties)).toBeCloseTo(
                probe.max_rotation_speed,
                6,
            );
            expect(publishedSpeed(nosedBlueprint.properties)).toBeCloseTo(
                miner.max_rotation_speed,
                6,
            );

            // 4. a saved orientation is a direction: +X stays +X, and (0, 2) faces +Y
            const plainState = expectOk(await plain.get_state(0), "plain state");
            const nosedState = expectOk(await nosed.get_state(0), "nosed state");
            expectNose(plainState.orientation, 0, 0.02);
            expectNose(nosedState.orientation, Math.PI / 2, 0.02);
        });
    },
);

test.skipIf(!hasServerBinary)(
    "turns along the shorter arc at no more than the maximum speed",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(shipConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins and the nose starts on +X
            const player = await login("player", "player");
            const ship = getShip(player, "plain");
            const spec = expectOk(await ship.get_specification(), "specification");
            const speed = spec.max_rotation_speed;
            expect(speed).toBeGreaterThan(0);
            const initial = expectOk(await ship.get_state(0), "initial state");
            expectNose(initial.orientation, 0, 0.02);
            await clock.stop();

            const target = -0.5;
            const durationMs = (Math.abs(target) / speed) * 1_000;
            let elapsedMs = 0;

            // 2. order a turn faster than the maximum
            const [status] = await ship.down_level("ship").run(async (session) => {
                expectStatus(
                    await session.send_rotate(Math.cos(target), Math.sin(target), speed * 10),
                    "send rotate",
                );
                const ack = session.wait_rotate_ack(PROCEED_TIMEOUT_MS);

                // 3. partway, the nose is on the short arc and the speed is the maximum
                await advance(clock, durationMs * 0.4);
                elapsedMs += durationMs * 0.4;
                expectStatus(await ack, "rotate ack");
                const mid = await readNose(session, clock, () => {
                    elapsedMs += 20;
                });
                expectNose(mid, -speed * (elapsedMs / 1_000));
                expect(Math.abs(angleDelta(noseAngle(mid), target))).toBeGreaterThan(0.05);

                // 4. the nose has not arrived before the shorter arc is done
                await advance(clock, durationMs * 0.35);
                elapsedMs += durationMs * 0.35;
                const before = await readNose(session, clock, () => {
                    elapsedMs += 20;
                });
                expect(Math.abs(angleDelta(noseAngle(before), target))).toBeGreaterThan(0.05);

                // 5. after that duration, the nose matches the target
                const remainingMs = durationMs - elapsedMs + 80;
                await advance(clock, remainingMs);
                elapsedMs += remainingMs;
                const arrived = await readNose(session, clock, () => {
                    elapsedMs += 20;
                });
                expectNose(arrived, target, 0.05);

                // 6. the server sends nothing when the turn finishes
                await new Promise((resolve) => setTimeout(resolve, 150));
                const [extra] = await session.wait_next(200);
                expect(extra.is_timeout(), extra.what()).toBe(true);

                return [Status.ok(), undefined];
            });
            expectStatus(status, "rotate");
        });
    },
);

test.skipIf(!hasServerBinary)(
    "takes the same direction when both arcs are equal",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(shipConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("player", "player");
            const ship = getShip(player, "plain");
            const spec = expectOk(await ship.get_specification(), "specification");
            const speed = spec.max_rotation_speed;
            await clock.stop();

            const halfTurnMs = (Math.PI / speed) * 1_000;

            // 2. order a half turn, from +X to -X
            const [status] = await ship.down_level("ship").run(async (session) => {
                expectStatus(
                    await session.send_rotate(-1, 0, speed),
                    "send rotate",
                );
                const ack = session.wait_rotate_ack(PROCEED_TIMEOUT_MS);

                // 3. halfway, the nose has passed through +Y
                await advance(clock, halfTurnMs * 0.5);
                expectStatus(await ack, "rotate ack");
                const mid = await readNose(session, clock);
                expectNose(mid, Math.PI / 2);

                // 4. the nose finishes on -X
                await advance(clock, halfTurnMs * 0.5 + 80);
                const arrived = await readNose(session, clock);
                expectNose(arrived, Math.PI, 0.05);

                return [Status.ok(), undefined];
            });
            expectStatus(status, "half turn");
        });
    },
);

test.skipIf(!hasServerBinary)(
    "a rotate during a turn leaves the first target",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(shipConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("player", "player");
            const ship = getShip(player, "plain");
            const spec = expectOk(await ship.get_specification(), "specification");
            const speed = spec.max_rotation_speed;
            await clock.stop();

            // 2. start a turn toward -Y
            const [status] = await ship.down_level("ship").run(async (session) => {
                expectStatus(
                    await session.send_rotate(0, -1, speed),
                    "send first rotate",
                );
                const firstAck = session.wait_rotate_ack(PROCEED_TIMEOUT_MS);
                await advance(clock, 400);
                expectStatus(await firstAck, "first rotate ack");
                const mid = await readNose(session, clock);
                const midAngle = noseAngle(mid);
                expect(midAngle).toBeLessThan(-0.2);
                expect(midAngle).toBeGreaterThan(-1.2);

                // 3. a second rotate aims back at +X
                expectStatus(
                    await session.send_rotate(1, 0, speed * 5),
                    "send second rotate",
                );
                const secondAck = session.wait_rotate_ack(PROCEED_TIMEOUT_MS);
                const returnMs = (Math.abs(midAngle) / speed) * 1_000;
                await advance(clock, returnMs + 80);
                expectStatus(await secondAck, "second rotate ack");
                const arrived = await readNose(session, clock);
                expectNose(arrived, 0, 0.08);

                return [Status.ok(), undefined];
            });
            expectStatus(status, "replaced turn");
        });
    },
);

function publishedSpeed(properties: { name: string; value: string }[]): number {
    const property = properties.find((item) => item.name === "max_rotation_speed");
    expect(property, "max_rotation_speed property").toBeDefined();
    const value = Number(property?.value);
    expect(Number.isFinite(value), property?.value).toBe(true);
    return value;
}

async function readNose(
    session: {
        send_state_request(): Promise<Status>;
        wait_state(timeout_ms?: number): Promise<[Status, { orientation: Nose } | undefined]>;
    },
    clock: IngameClock,
    onSample?: () => void,
): Promise<Nose> {
    expectStatus(await session.send_state_request(), "state request");
    const pending = session.wait_state(PROCEED_TIMEOUT_MS);
    await advance(clock, 20);
    onSample?.();
    const [status, state] = await pending;
    expectStatus(status, "state");
    expect(state, "state").toBeDefined();
    return state!.orientation;
}

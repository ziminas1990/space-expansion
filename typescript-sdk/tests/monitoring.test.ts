import { expect, test } from "vitest";
import type { Status } from "../types/status.js";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    General,
    makeMiner,
    Player,
    Position,
    World,
} from "./configurator/index.js";
import {
    hasServerBinary,
    integrationTimeoutMs,
    withServer,
} from "./fixture.js";
import {
    expectStatus,
    getCargo,
    getRCS,
    getPassiveScanner,
    getShip,
    getSystemClock,
} from "./helpers/index.js";

const HEARTBEAT_MS = 80;
const IDLE_STOP_COUNT = 3;
const SERVER_PERIOD_MS = 10_000;

function monitoringConfiguration(): Configuration {
    return new Configuration({
        general: new General({
            totalThreads: 1,
            loginUdpPort: 7_456,
            initialState: ApplicationMode.Run,
            portsPool: [12_000, 12_100],
        }),
        blueprints: new DefaultBlueprints(),
        world: new World(),
        players: [
            new Player({
                login: "player",
                password: "player",
                ships: [makeMiner("miner-1", new Position(0, 0))],
            }),
        ],
    });
}

async function collectIdleAndStop<T>(
    start: (
        callback: (payload: T | undefined) => Promise<boolean>,
    ) => Promise<Status>,
    description: string,
): Promise<number[]> {
    const idleAt: number[] = [];
    const startedAt = performance.now();
    expectStatus(
        await start(async (payload) => {
            if (payload === undefined) {
                idleAt.push(performance.now());
            }
            return idleAt.length < IDLE_STOP_COUNT;
        }),
        description,
    );
    expect(
        performance.now() - startedAt,
        `${description} stopped before the server period`,
    ).toBeLessThan(SERVER_PERIOD_MS / 2);
    return idleAt;
}

function expectIdleCadence(idleAt: number[], description: string): void {
    expect(idleAt.length, `${description} idle count`).toBe(IDLE_STOP_COUNT);
    for (let i = 1; i < idleAt.length; i += 1) {
        const dt = idleAt[i]! - idleAt[i - 1]!;
        expect(dt, `${description} idle gap ${i}`).toBeGreaterThanOrEqual(
            HEARTBEAT_MS * 0.4,
        );
        expect(dt, `${description} idle gap ${i}`).toBeLessThan(HEARTBEAT_MS * 4);
    }
}

test.skipIf(!hasServerBinary)(
    "invokes monitoring callbacks on idle heartbeat and stops when they return false",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(monitoringConfiguration(), async ({ login }) => {
            // 1. player logins
            const player = await login("player", "player");

            // 2. get ship modules used for monitoring
            const miner = getShip(player, "miner-1");
            const cargo = getCargo(miner, "cargo");
            const engine = getRCS(miner, "main_rcs");
            const scanner = getPassiveScanner(miner, "perceiver");
            const systemClock = getSystemClock(player);

            // 3. start monitoring on every midlevel module with a short heartbeat
            const commutatorIdle = collectIdleAndStop(
                (callback) => player.down_level().monitoring(callback, HEARTBEAT_MS),
                "commutator monitoring",
            );
            const shipIdle = collectIdleAndStop(
                (callback) => miner.down_level("ship").monitoring(
                    SERVER_PERIOD_MS, callback, HEARTBEAT_MS),
                "ship monitoring",
            );
            const cargoIdle = collectIdleAndStop(
                (callback) => cargo.down_level().monitoring(callback, HEARTBEAT_MS),
                "cargo monitoring",
            );
            const engineIdle = collectIdleAndStop(
                (callback) => engine.down_level().monitoring(callback, HEARTBEAT_MS),
                "engine monitoring",
            );
            const scannerIdle = collectIdleAndStop(
                (callback) => scanner.down_level().monitoring(callback, HEARTBEAT_MS),
                "scanner monitoring",
            );
            const clockIdle = collectIdleAndStop(
                (callback) => systemClock.down_level().monitoring(
                    SERVER_PERIOD_MS, callback, HEARTBEAT_MS),
                "clock monitoring",
            );

            // 4. wait for each loop to stop after idle callbacks
            const [commutatorAt, shipAt, cargoAt, engineAt, scannerAt, clockAt] =
                await Promise.all([
                    commutatorIdle,
                    shipIdle,
                    cargoIdle,
                    engineIdle,
                    scannerIdle,
                    clockIdle,
                ]);

            // 5. check idle callbacks arrived at about the heartbeat interval
            expectIdleCadence(commutatorAt, "commutator");
            expectIdleCadence(shipAt, "ship");
            expectIdleCadence(cargoAt, "cargo");
            expectIdleCadence(engineAt, "engine");
            expectIdleCadence(scannerAt, "scanner");
            expectIdleCadence(clockAt, "clock");
        });
    },
);

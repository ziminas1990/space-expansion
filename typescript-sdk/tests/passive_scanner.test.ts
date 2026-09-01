import { expect, test } from "vitest";
import type { PassiveScanner, SystemClock } from "../highlevel/index.js";
import type { PhysicalObject } from "../types/index.js";
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
    distance,
    expectOk,
    expectStatus,
    getPassiveScanner,
    getShip,
    getSystemClock,
    Randomizer,
} from "./helpers/index.js";

function passiveScannerConfiguration(): Configuration {
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
                login: "oreman",
                password: "thinbones",
                ships: [makeMiner("miner-1", new Position(0, 0))],
            }),
        ],
    });
}

async function scanning(
    scanner: PassiveScanner,
    clock: SystemClock,
    scanningTimeMs: number,
): Promise<Map<number, PhysicalObject>> {
    const startAtUs = expectOk(await clock.time(), "scan start time");
    const endAtUs = startAtUs + BigInt(scanningTimeMs) * 1_000n;
    const scanningResult = new Map<number, PhysicalObject>();

    expectStatus(
        await scanner.down_level().monitoring(async (objects) => {
            if (objects) {
                for (const detected of objects) {
                    scanningResult.set(detected.object_id, detected);
                }
            }
            const nowUs = expectOk(await clock.time(), "scan now");
            return nowUs <= endAtUs;
        }),
        "scanner monitoring",
    );

    return scanningResult;
}

test.skipIf(!hasServerBinary)(
    "returns the passive scanner specification",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(passiveScannerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("oreman", "thinbones");

            // 2. get miner ship and scanner
            const scanner = getPassiveScanner(getShip(player, "miner-1"), "perceiver");

            // 3. get specification
            expectOk(await scanner.get_specification(), "scanner specification");
        });
    },
);

test.skipIf(!hasServerBinary)(
    "scans spawned asteroids within scanning radius",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(passiveScannerConfiguration(), async ({
            administrator,
            login,
            clock,
        }) => {
            const randomizer = new Randomizer(3_284);
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("oreman", "thinbones");

            // 2. get miner ship, scanner, specification, and system clock
            const systemClock = getSystemClock(player);
            const miner = getShip(player, "miner-1");
            const scanner = getPassiveScanner(miner, "perceiver");
            const spec = expectOk(
                await scanner.get_specification(),
                "scanner specification",
            );

            // 3. get ship position
            const shipPosition = expectOk(await miner.get_position(), "ship position");

            // 4. spawn asteroids around the ship
            const spawnedAsteroids: PhysicalObject[] = [];
            for (let i = 0; i < 1_000; i += 1) {
                spawnedAsteroids.push(expectOk(
                    await administrator.spawner.spawn_asteroid(
                        randomizer.randomPosition({
                            center: shipPosition,
                            radius: 2 * spec.scanning_radius_km * 1_000,
                        }),
                        { ice: 100, metals: 32 },
                        randomizer.randomValue(5, 20),
                    ),
                    `spawn asteroid ${i}`,
                ));
            }

            // 5. scan for 10 seconds
            const scannedObjects = await scanning(scanner, systemClock, 10_000);

            // 6. check that asteroids within scanning radius were detected
            for (const candidate of spawnedAsteroids) {
                if (distance(shipPosition, candidate.position)
                    <= spec.scanning_radius_km * 1_000)
                {
                    expect(
                        scannedObjects.has(candidate.object_id),
                        `asteroid ${candidate.object_id} within range`,
                    ).toBe(true);
                }
            }
        });
    },
);

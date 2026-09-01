import { expect, test } from "vitest";
import type { PhysicalObject, Position, Status } from "../types/index.js";
import { predict_position } from "../utils/predictor.js";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    General,
    Player,
    World,
} from "./configurator/index.js";
import {
    hasServerBinary,
    integrationTimeoutMs,
    withServer,
} from "./fixture.js";
import {
    almostEqualPosition,
    collectEvent,
    expectOk,
    getAllShips,
    getShip,
    Randomizer,
} from "./helpers/index.js";

function administratorConfiguration(): Configuration {
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
                login: "spy007",
                password: "iamspy",
                ships: [],
            }),
        ],
    });
}

function sleep(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

function expectError(status: Status, code: string): void {
    expect(status.what()).toBe(code);
}

function spawnAreaPosition(randomizer: Randomizer): Position {
    return randomizer.randomPosition({
        rect: { left: -1_000, right: 1_000, bottom: -1_000, top: 1_000 },
        minSpeed: 0,
        maxSpeed: 1_000,
    });
}

test.skipIf(!hasServerBinary)(
    "keeps frozen time at zero",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(administratorConfiguration(), async ({ clock }) => {
            // 1. check that frozen time stays at zero
            for (let i = 0; i < 3; i += 1) {
                // 1.1 read ingame time and expect zero
                const ingameTime = await clock.time();
                expect(ingameTime, `iteration ${i}`).toBe(0n);

                // 1.2 wait on wall clock
                await sleep(100);
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "proceeds frozen time by the requested interval",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(administratorConfiguration(), async ({ clock }) => {
            // 1. check frozen time is zero
            const ingameTime = await clock.time();
            expect(ingameTime).toBe(0n);

            // 2. proceed frozen time by 2000ms
            const newIngameTime = await clock.proceed(2_000, 1_000);
            const timeDelta = newIngameTime - ingameTime;

            // 3. check time advanced by ~2000000us
            expect(timeDelta).toBeGreaterThanOrEqual(1_999_000n);
            expect(timeDelta).toBeLessThanOrEqual(2_001_000n);
        });
    },
);

test.skipIf(!hasServerBinary)(
    "switches between real-time and debug modes",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(administratorConfiguration(), async ({ clock }) => {
            // 1. check frozen time is zero
            const ingameTime = await clock.time();
            expect(ingameTime).toBe(0n);

            // 2. switch to real-time and wait 1s
            await clock.play();
            await sleep(1_000);

            // 3. stop clock and check ~1s elapsed
            const stoppedAt = await clock.stop();
            // Rude check with 5% accuracy.
            expect(stoppedAt).toBeGreaterThanOrEqual(950_000n);
            expect(stoppedAt).toBeLessThanOrEqual(1_050_000n);
        });
    },
);

test.skipIf(!hasServerBinary)(
    "spawns asteroids and returns an id with a current timestamp",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(administratorConfiguration(), async ({ administrator, clock }) => {
            await clock.play();
            const randomizer = new Randomizer(2_132);
            const { spawner } = administrator;

            // 1. spawn asteroids and check timestamps
            for (let i = 0; i < 100; i += 1) {
                // 1.1 read current ingame time
                const now = await clock.time();

                // 1.2 spawn an asteroid
                const asteroid = expectOk(
                    await spawner.spawn_asteroid(
                        spawnAreaPosition(randomizer),
                        { ice: 100, metals: 32 },
                        10,
                    ),
                    `spawn asteroid ${i}`,
                );

                // 1.3 check spawn timestamp is not in the past
                expect(asteroid.position.timestamp).toBeGreaterThanOrEqual(now);
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "spawns ships that manipulator can look up at the predicted position",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(administratorConfiguration(), async ({ administrator, clock }) => {
            await clock.play();
            const randomizer = new Randomizer(2_132);
            const { spawner, manipulator } = administrator;
            const spawnedShips: PhysicalObject[] = [];

            // 1. spawn ships
            for (let i = 0; i < 100; i += 1) {
                const ship = expectOk(
                    await spawner.spawn_ship(
                        "spy007",
                        "Ship/Miner",
                        `Miner_${i}`,
                        spawnAreaPosition(randomizer),
                    ),
                    `spawn ship ${i}`,
                );
                spawnedShips.push(ship);
            }

            // 2. look up spawned ships and check positions
            for (const spawned of spawnedShips) {
                // 2.1 get the ship via manipulator
                const found = expectOk(
                    await manipulator.get_object(
                        spawned.object_type,
                        spawned.object_id,
                    ),
                    `get ship ${spawned.object_id}`,
                );

                // 2.2 check predicted position matches
                expect(
                    almostEqualPosition(
                        predict_position(spawned.position, found.position.timestamp),
                        found.position,
                    ),
                    `ship ${spawned.object_id} position`,
                ).toBe(true);
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "notifies a logged-in player when an administrator spawns a ship",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(administratorConfiguration(), async ({
            administrator,
            clock,
            login,
        }) => {
            await clock.play();

            // 1. player logins and waits for ship to be attached
            const player = await login("spy007", "iamspy");
            expect(getAllShips(player)).toEqual([]);

            const attached = collectEvent(player, "ship_attached");

            // 2. administrator spawns a new ship
            const spawnPosition: Position = {
                timestamp: 0n,
                point: [1_500, -2_400],
                velocity: [12, -7],
            };
            const spawned = expectOk(
                await administrator.spawner.spawn_ship(
                    "spy007",
                    "Ship/Miner",
                    "Miner",
                    spawnPosition,
                ),
                "spawn ship",
            );

            // 3. player receives ship attached event
            await attached.waitForCount(1, "ship attached");
            expect(attached.items[0]?.name).toBe("Miner");
            expect(getAllShips(player).map((item) => item.name)).toEqual(["Miner"]);

            // 4. player gets ship position and checks if it matches the
            //    administrator spawn position
            const position = expectOk(
                await getShip(player, "Miner").get_position(),
                "player ship position",
            );
            expect(
                almostEqualPosition(
                    predict_position(spawned.position, position.timestamp),
                    position,
                ),
                "player ship position matches administrator spawn",
            ).toBe(true);
        });
    },
);

test.skipIf(!hasServerBinary)(
    "rejects spawn_ship for missing blueprint, missing player, and non-ship blueprint",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(administratorConfiguration(), async ({ administrator, clock }) => {
            await clock.play();
            const { spawner } = administrator;
            const position: Position = {
                timestamp: 0n,
                point: [0, 0],
                velocity: [0, 0],
            };

            // 1. spawn with a missing blueprint
            {
                const [status, ship] = await spawner.spawn_ship(
                    "spy007",
                    "Ship/dgfdg",
                    "Miner",
                    position,
                );
                expectError(status, "BLUEPRINT_DOESNT_EXIST");
                expect(ship).toBeUndefined();
            }

            // 2. spawn for a missing player
            {
                const [status, ship] = await spawner.spawn_ship(
                    "spy0fd07",
                    "Ship/Miner",
                    "Miner",
                    position,
                );
                expectError(status, "PLAYER_DOESNT_EXIST");
                expect(ship).toBeUndefined();
            }

            // 3. spawn with a non-ship blueprint
            {
                const [status, ship] = await spawner.spawn_ship(
                    "spy007",
                    "PassiveScanner/Military Scanner",
                    "Miner",
                    position,
                );
                expectError(status, "NOT_A_SHIP_BLUEPRINT");
                expect(ship).toBeUndefined();
            }
        });
    },
);

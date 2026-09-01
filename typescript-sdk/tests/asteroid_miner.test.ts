import { expect, test } from "vitest";
import type {
    AsteroidMiner,
    ResourceContainerContent,
    Ship,
    SystemClock,
} from "../highlevel/index.js";
import type { PhysicalObject, Status } from "../types/index.js";
import {
    ApplicationMode,
    Asteroid,
    Asteroids,
    Configuration,
    DefaultBlueprints,
    General,
    makeMiner,
    Player,
    Position,
    ResourceType,
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
    getAsteroidMiner,
    getCargo,
    getPassiveScanner,
    getShip,
    getSystemClock,
} from "./helpers/index.js";

const toyMiner = {
    max_distance: 500,
    cycle_time_ms: 10_000,
    yield_per_cycle: 250,
};

function asteroidMinerConfiguration(): Configuration {
    return new Configuration({
        general: new General({
            totalThreads: 1,
            loginUdpPort: 7_456,
            initialState: ApplicationMode.Freeze,
            portsPool: [12_000, 12_100],
        }),
        blueprints: new DefaultBlueprints(),
        world: new World(new Asteroids([
            new Asteroid({
                position: new Position(100, 100),
                radius: 10,
                composition: {
                    [ResourceType.Ice]: 15,
                    [ResourceType.Silicates]: 15,
                    [ResourceType.Metals]: 20,
                    [ResourceType.Stones]: 50,
                },
            }),
            new Asteroid({
                position: new Position(5_000, 5_000),
                radius: 20,
                composition: {
                    [ResourceType.Ice]: 10,
                    [ResourceType.Silicates]: 10,
                    [ResourceType.Metals]: 20,
                    [ResourceType.Stones]: 60,
                },
            }),
        ])),
        players: [
            new Player({
                login: "oreman",
                password: "thinbones",
                ships: [makeMiner("miner-1", new Position(0, 0))],
            }),
        ],
    });
}

function expectError(status: Status, code: string): void {
    expect(status.what()).toBe(code);
}

function resourceAmount(
    content: ResourceContainerContent,
    resourceType: string,
): number {
    return content.resources.find((item) => item.resource_type === resourceType)
        ?.amount ?? 0;
}

function track<T>(promise: Promise<T>): { done: boolean; promise: Promise<T> } {
    const tracked = { done: false, promise };
    void promise.finally(() => {
        tracked.done = true;
    });
    return tracked;
}

async function withTimeout<T>(
    promise: Promise<T>,
    timeoutMs: number,
    description: string,
): Promise<T> {
    let timer: ReturnType<typeof setTimeout> | undefined;
    try {
        return await Promise.race([
            promise,
            new Promise<never>((_, reject) => {
                timer = setTimeout(
                    () => reject(new Error(`${description} timed out`)),
                    timeoutMs,
                );
            }),
        ]);
    } finally {
        if (timer !== undefined) {
            clearTimeout(timer);
        }
    }
}

async function findAsteroids(
    minerShip: Ship,
    clock: SystemClock,
): Promise<{ near: PhysicalObject; far: PhysicalObject }> {
    const scanner = getPassiveScanner(minerShip, "perceiver");
    const shipPosition = expectOk(await minerShip.get_position(), "ship position");
    const startAtUs = expectOk(await clock.time(), "scan start time");
    const deadlineUs = startAtUs + 10_000n * 1_000n;
    const scanned = new Map<number, PhysicalObject>();

    expectStatus(
        await scanner.down_level().monitoring(async (objects) => {
            if (objects) {
                for (const detected of objects) {
                    if (detected.object_type === "asteroid") {
                        scanned.set(detected.object_id, detected);
                    }
                }
            }
            const nowUs = expectOk(await clock.time(), "scan now");
            return scanned.size < 2 && nowUs <= deadlineUs;
        }),
        "scan asteroids",
    );

    let near: PhysicalObject | undefined;
    let far: PhysicalObject | undefined;
    for (const asteroid of scanned.values()) {
        const range = distance(shipPosition, asteroid.position);
        if (range < 1_000) {
            near = asteroid;
        } else if (range > 3_000) {
            far = asteroid;
        }
    }
    if (near === undefined || far === undefined) {
        throw new Error(
            `failed to locate asteroids (near=${near?.object_id}, far=${far?.object_id})`,
        );
    }
    return { near, far };
}

async function bindToCargo(miner: AsteroidMiner, cargoName: string): Promise<void> {
    expectStatus(await miner.bind_to_cargo(cargoName), `bind to ${cargoName}`);
    expect(miner.cargo_name).toBe(cargoName);
}

test.skipIf(!hasServerBinary)(
    "returns the asteroid miner specification",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(asteroidMinerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("oreman", "thinbones");

            // 2. get miner ship and asteroid miner
            const miner = getAsteroidMiner(getShip(player, "miner-1"), "miner");

            // 3. get specification
            const spec = expectOk(
                await miner.get_specification(),
                "miner specification",
            );
            expect(spec.max_distance).toBe(toyMiner.max_distance);
            expect(spec.cycle_time_ms).toBe(toyMiner.cycle_time_ms);
            expect(spec.yield_per_cycle).toBe(toyMiner.yield_per_cycle);
        });
    },
);

test.skipIf(!hasServerBinary)(
    "binds to a cargo container",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(asteroidMinerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("oreman", "thinbones");

            // 2. get miner ship and asteroid miner
            const miner = getAsteroidMiner(getShip(player, "miner-1"), "miner");

            // 3. bind to an invalid cargo
            expectError(
                await miner.bind_to_cargo("invalid cargo"),
                "NOT_BOUND_TO_CARGO",
            );
            expect(miner.cargo_name).toBeUndefined();

            // 4. bind to cargo
            await bindToCargo(miner, "cargo");
        });
    },
);

test.skipIf(!hasServerBinary)(
    "rejects mining when the asteroid is missing or too far",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(asteroidMinerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(50);

            // 1. player logins
            const player = await login("oreman", "thinbones");

            // 2. get miner ship and asteroid miner
            const minerShip = getShip(player, "miner-1");
            const miner = getAsteroidMiner(minerShip, "miner");

            // 3. bind to cargo
            await bindToCargo(miner, "cargo");

            // 4. start mining a missing asteroid
            expectError(
                await miner.down_level().start_mining(
                    100_500,
                    async () => false,
                ),
                "ASTEROID_DOESNT_EXIST",
            );

            // 5. find nearby and distant asteroids
            const { near, far } = await findAsteroids(
                minerShip,
                getSystemClock(player),
            );

            // 6. start mining the distant asteroid
            expectError(
                await miner.down_level().start_mining(
                    far.object_id,
                    async () => false,
                ),
                "ASTEROID_TOO_FAR",
            );

            // 7. start mining the nearby asteroid
            expectStatus(
                await miner.down_level().start_mining(
                    near.object_id,
                    async () => false,
                ),
                "start mining nearby asteroid",
            );
        });
    },
);

test.skipIf(!hasServerBinary)(
    "mines metals into cargo",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(asteroidMinerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(50);

            // 1. player logins
            const player = await login("oreman", "thinbones");

            // 2. get miner ship and asteroid miner
            const minerShip = getShip(player, "miner-1");
            const miner = getAsteroidMiner(minerShip, "miner");

            // 3. bind to cargo
            await bindToCargo(miner, "cargo");

            // 4. find a nearby asteroid
            const { near } = await findAsteroids(minerShip, getSystemClock(player));

            // 5. mine until 1000 metals are collected
            const collected = new Map<string, number>([["metals", 0]]);
            await clock.fastForward(1_000, 10_000);
            expectStatus(
                await miner.down_level().start_mining(
                    near.object_id,
                    async (resources) => {
                        for (const item of resources) {
                            collected.set(
                                item.resource_type,
                                (collected.get(item.resource_type) ?? 0) + item.amount,
                            );
                        }
                        return (collected.get("metals") ?? 0) < 1_000;
                    },
                ),
                "mine metals",
            );
            await clock.fastForward(50);

            // 6. check cargo metals
            const content = expectOk(
                await getCargo(minerShip, "cargo").get_content(0),
                "cargo content",
            );
            expect(resourceAmount(content, "metals")).toBeGreaterThanOrEqual(1_000);
        });
    },
);

test.skipIf(!hasServerBinary)(
    "stops mining when requested",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(asteroidMinerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(50);

            // 1. player logins
            const player = await login("oreman", "thinbones");

            // 2. get miner ship and asteroid miner
            const minerShip = getShip(player, "miner-1");
            const miner = getAsteroidMiner(minerShip, "miner");

            // 3. get specification
            const spec = expectOk(
                await miner.get_specification(),
                "miner specification",
            );

            // 4. bind to cargo
            await bindToCargo(miner, "cargo");

            // 5. stop mining while idle
            expectError(await miner.stop_mining(), "MINER_IS_IDLE");

            // 6. find a nearby asteroid
            const { near } = await findAsteroids(minerShip, getSystemClock(player));

            // 7. start mining
            const mining = track(
                miner.down_level().start_mining(near.object_id, async () => true),
            );

            // 8. proceed through several mining cycles
            await clock.proceed(Math.trunc(spec.cycle_time_ms * 5.5), 5_000);
            expect(mining.done).toBe(false);

            // 9. stop mining
            await clock.fastForward(50);
            expectStatus(await miner.stop_mining(), "stop mining");

            // 10. wait for the mining task to finish
            expectError(
                await withTimeout(mining.promise, 1_000, "mining task"),
                "INTERRUPTED_BY_USER",
            );
        });
    },
);

test.skipIf(!hasServerBinary)(
    "stops mining when cargo is full",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(asteroidMinerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(50);

            // 1. player logins
            const player = await login("oreman", "thinbones");

            // 2. get miner ship and asteroid miner
            const minerShip = getShip(player, "miner-1");
            const miner = getAsteroidMiner(minerShip, "miner");

            // 3. get specification
            expectOk(await miner.get_specification(), "miner specification");

            // 4. bind to tiny_cargo
            await bindToCargo(miner, "tiny_cargo");

            // 5. find a nearby asteroid
            const { near } = await findAsteroids(minerShip, getSystemClock(player));

            // 6. mine until cargo is full
            await clock.fastForward(1_000, 10_000);
            expectError(
                await miner.down_level().start_mining(
                    near.object_id,
                    async () => true,
                ),
                "NO_SPACE_AVAILABLE",
            );

            // 7. check that cargo is full
            const content = expectOk(
                await getCargo(minerShip, "tiny_cargo").get_content(0),
                "tiny cargo content",
            );
            expect(content.used).toBeCloseTo(content.volume, 5);
        });
    },
);

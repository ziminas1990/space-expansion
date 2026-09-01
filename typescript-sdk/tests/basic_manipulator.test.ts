import { expect, test } from "vitest";
import type { PhysicalObject, Position } from "../types/index.js";
import { predict_position } from "../utils/predictor.js";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    General,
    World,
} from "./configurator/index.js";
import {
    hasServerBinary,
    integrationTimeoutMs,
    withServer,
} from "./fixture.js";
import {
    almostEqualPosition,
    expectOk,
    Randomizer,
} from "./helpers/index.js";

const asteroidCount = 500;
const oneMinuteUs = 60n * 1_000_000n;

function basicManipulatorConfiguration(): Configuration {
    return new Configuration({
        general: new General({
            totalThreads: 1,
            loginUdpPort: 7_456,
            initialState: ApplicationMode.Run,
            portsPool: [12_000, 12_100],
        }),
        blueprints: new DefaultBlueprints(),
        world: new World(),
        players: {},
    });
}

function spawnAreaPosition(randomizer: Randomizer): Position {
    return randomizer.randomPosition({
        rect: { left: -1_000, right: 1_000, bottom: -1_000, top: 1_000 },
        minSpeed: 0,
        maxSpeed: 1_000,
    });
}

test.skipIf(!hasServerBinary)(
    "looks up spawned asteroids at the predicted position",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(basicManipulatorConfiguration(), async ({
            administrator,
        }) => {
            const randomizer = new Randomizer(2_132);
            const { spawner, manipulator } = administrator;
            const spawnedAsteroids: PhysicalObject[] = [];

            // 1. spawn asteroids
            for (let i = 0; i < asteroidCount; i += 1) {
                spawnedAsteroids.push(expectOk(
                    await spawner.spawn_asteroid(
                        spawnAreaPosition(randomizer),
                        { ice: 100 },
                        10,
                    ),
                    `spawn asteroid ${i}`,
                ));
            }

            // 2. look up spawned asteroids and check positions
            for (const spawned of spawnedAsteroids) {
                // 2.1 get the asteroid via manipulator
                const found = expectOk(
                    await manipulator.get_object(
                        spawned.object_type,
                        spawned.object_id,
                    ),
                    `get asteroid ${spawned.object_id}`,
                );

                // 2.2 check predicted position matches
                expect(
                    almostEqualPosition(
                        predict_position(
                            spawned.position,
                            found.position.timestamp,
                        ),
                        found.position,
                    ),
                    `asteroid ${spawned.object_id} position`,
                ).toBe(true);
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "moves spawned asteroids to a predicted position after one minute",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(basicManipulatorConfiguration(), async ({
            administrator,
        }) => {
            const randomizer = new Randomizer(3_254);
            const { spawner, manipulator } = administrator;
            const spawnedAsteroids: PhysicalObject[] = [];

            // 1. spawn asteroids
            for (let i = 0; i < asteroidCount; i += 1) {
                spawnedAsteroids.push(expectOk(
                    await spawner.spawn_asteroid(
                        spawnAreaPosition(randomizer),
                        { ice: 100 },
                        10,
                    ),
                    `spawn asteroid ${i}`,
                ));
            }

            // 2. move each asteroid to its predicted position after 1 minute
            for (const spawned of spawnedAsteroids) {
                // 2.1 get the asteroid via manipulator
                const asteroid = expectOk(
                    await manipulator.get_object(
                        spawned.object_type,
                        spawned.object_id,
                    ),
                    `get asteroid ${spawned.object_id}`,
                );

                // 2.2 move to predicted position after 1 minute
                const movedTo = predict_position(
                    asteroid.position,
                    asteroid.position.timestamp + oneMinuteUs,
                );
                const newPosition = expectOk(
                    await manipulator.move_object(
                        asteroid.object_type,
                        asteroid.object_id,
                        movedTo,
                    ),
                    `move asteroid ${asteroid.object_id}`,
                );

                // 2.3 get the asteroid again
                const found = expectOk(
                    await manipulator.get_object(
                        spawned.object_type,
                        spawned.object_id,
                    ),
                    `get moved asteroid ${spawned.object_id}`,
                );

                // 2.4 check predicted position matches
                expect(
                    almostEqualPosition(
                        predict_position(newPosition, found.position.timestamp),
                        found.position,
                    ),
                    `asteroid ${spawned.object_id} moved position`,
                ).toBe(true);
            }
        });
    },
);

import { expect, test } from "vitest";
import type { Position } from "../types/index.js";
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
import { ModuleType } from "../highlevel/index.js";

function commutatorConfiguration(): Configuration {
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
                password: "expansion",
                ships: [],
            }),
        ],
    });
}

test.skipIf(!hasServerBinary)(
    "attaches spawned ships to the player at the predicted position",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(commutatorConfiguration(), async ({
            administrator,
            login,
        }) => {
            const randomizer = new Randomizer(4_934);

            // 1. player logins
            const player = await login("player", "expansion");
            expect(getAllShips(player)).toEqual([]);

            const attached = collectEvent(player, "ship_attached");

            // 2. spawn ships and check they appear on the player
            for (let i = 0; i < 10; i += 1) {
                const shipName = `Miner_#${i}`;

                // 2.1 administrator spawns a ship
                const spawnPosition: Position = randomizer.randomPosition({
                    center: {
                        timestamp: 0n,
                        point: [0, 0],
                        velocity: [0, 0],
                    },
                    radius: 100_000,
                });
                const spawned = expectOk(
                    await administrator.spawner.spawn_ship(
                        "player",
                        "Ship/Miner",
                        shipName,
                        spawnPosition,
                    ),
                    `spawn ${shipName}`,
                );

                // 2.2 wait for the ship attached event
                await attached.waitFor(
                    (items) => items.length > 0,
                    `${shipName} attached`,
                );
                const eventShip = attached.take();
                expect(eventShip?.name).toBe(shipName);
                expect(eventShip?.type).toBe(ModuleType.SHIP);
                expect(eventShip?.ship_class).toBe("Ship/Miner");

                // 2.3 look up the ship in the player registry
                const ship = getShip(player, shipName);
                expect(ship).toBe(eventShip);

                // 2.4 check predicted position matches
                const position = expectOk(
                    await ship.get_position(),
                    `${shipName} position`,
                );
                expect(
                    almostEqualPosition(
                        predict_position(spawned.position, position.timestamp),
                        position,
                    ),
                    `${shipName} position matches spawn`,
                ).toBe(true);
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "reports a spawned ship as type Ship with its blueprint name",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(commutatorConfiguration(), async ({
            administrator,
            login,
        }) => {
            // 1. player logins
            const player = await login("player", "expansion");
            const attached = collectEvent(player, "ship_attached");

            // 2. administrator spawns a ship without the client opening a
            //    tunnel to it first
            expectOk(
                await administrator.spawner.spawn_ship(
                    "player",
                    "Ship/Miner",
                    "Miner-1",
                    {
                        timestamp: 0n,
                        point: [0, 0],
                        velocity: [0, 0],
                    },
                ),
                "spawn Miner-1",
            );

            // 3. wait until the ship is listed on the root commutator
            await attached.waitForCount(1, "Miner-1 attached");

            // 4. read module info from the commutator (no ship specification)
            const modules = expectOk(
                await player.down_level("root_commutator").get_all_modules_info(),
                "list attached modules",
            );
            const shipInfo = modules.find((info) => info.module_name === "Miner-1");
            expect(shipInfo?.module_type).toBe(ModuleType.SHIP);
            expect(shipInfo?.module_name).toBe("Miner-1");
            expect(shipInfo?.blueprint_name).toBe("Ship/Miner");

            // 5. the highlevel ship uses the same blueprint as ship_class
            const ship = getShip(player, "Miner-1");
            expect(ship.type).toBe(ModuleType.SHIP);
            expect(ship.ship_class).toBe("Ship/Miner");
        });
    },
);


import { expect, test } from "vitest";
import {
    ApplicationMode,
    Asteroid,
    Asteroids,
    Configuration,
    DefaultBlueprints,
    General,
    makeProbe,
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

function loginConfiguration(): Configuration {
    return new Configuration({
        general: new General({
            totalThreads: 1,
            loginUdpPort: 7_456,
            initialState: ApplicationMode.Run,
            portsPool: [12_000, 12_100],
        }),
        blueprints: new DefaultBlueprints(),
        world: new World(
            new Asteroids([
                new Asteroid({
                    position: new Position(10_000, 10_000),
                    radius: 200,
                    composition: {
                        [ResourceType.Ice]: 20,
                        [ResourceType.Silicates]: 20,
                        [ResourceType.Metals]: 20,
                    },
                }),
            ]),
        ),
        players: [
            new Player({
                login: "spy007",
                password: "iamspy",
                ships: [makeProbe("scout-1", new Position(100, 200))],
            }),
        ],
    });
}

test.skipIf(!hasServerBinary)(
    "logs in as a configured player",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(loginConfiguration(), async ({ login }) => {
            const player = await login("spy007", "iamspy");
            expect(player).toBeTruthy();
            expect(player.down_level("root_commutator")).toBeTruthy();
        });
    },
);

test.skipIf(!hasServerBinary)(
    "opens 8 simultaneous sessions for the same player",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(loginConfiguration(), async ({ login }) => {
            // 8 is a hardcoded server's persistent-session limit (per player).
            for (let i = 0; i < 8; i += 1) {
                try {
                    const player = await login("spy007", "iamspy");
                    expect(player).toBeTruthy();
                } catch (error: unknown) {
                    const message = error instanceof Error
                        ? error.message
                        : String(error);
                    throw new Error(`${message} at iteration ${i}`);
                }
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "survives 100 login/release cycles",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(loginConfiguration(), async ({ login }) => {
            for (let i = 0; i < 100; i += 1) {
                try {
                    const player = await login("spy007", "iamspy");
                    expect(player).toBeTruthy();
                    await player.release();
                } catch (error: unknown) {
                    const message = error instanceof Error
                        ? error.message
                        : String(error);
                    throw new Error(`${message} at iteration ${i}`);
                }
            }
        });
    },
);

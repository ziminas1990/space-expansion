import { expect, test } from "vitest";
import { Game, ModuleType } from "../midlevel/index.js";
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
import { expectOk, expectStatus } from "./helpers/index.js";

function gameConfiguration(): Configuration {
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
                ships: [],
            }),
        ],
    });
}

test.skipIf(!hasServerBinary)(
    "opens Game from the root commutator and monitors updates",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(gameConfiguration(), async ({ login }) => {
            // 1. player logins
            const player = await login("player", "player");

            // 2. game is listed on the root commutator
            const modules = expectOk(
                await player.down_level().get_all_modules_info(),
                "list modules",
            );
            const info = modules.find((item) => item.module_type === ModuleType.GAME);
            if (info === undefined) {
                throw new Error("Game not found");
            }
            expect(info.module_name).toBe("Game");

            // 3. start monitoring on a dedicated session
            const game = new Game(info.open_session_cb);
            const heartbeats: number[] = [];
            const monitoring = game.monitoring(async () => {
                heartbeats.push(Date.now());
                return heartbeats.length < 2;
            });

            // 4. heartbeats arrive on the game session until monitoring stops
            expectStatus(await monitoring);
            expect(heartbeats.length).toBeGreaterThanOrEqual(2);
        });
    },
);

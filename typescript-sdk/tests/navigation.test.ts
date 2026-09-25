import { expect, test } from "vitest";
import { follow_flight_plan } from "../highlevel/index.js";
import { build_plan } from "../utils/index.js";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    General,
    makeProbe,
    Player,
    Position,
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
    FastForwardClock,
    getHoverEngine,
    getShip,
    getSystemClock,
} from "./helpers/index.js";

function navigationConfiguration(): Configuration {
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
                login: "spy007",
                password: "iamspy",
                ships: [
                    makeProbe(
                        "scout-1",
                        new Position(100, 200, new Vector(100, -100)),
                    ),
                    makeProbe(
                        "scout-2",
                        new Position(-100, -200, new Vector(-10, 20)),
                    ),
                ],
            }),
        ],
    });
}

test.skipIf(!hasServerBinary)(
    "moves scout-1 to intercept scout-2",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(navigationConfiguration(), async ({ login, clock }) => {
            // 1. player logins
            const player = await login("spy007", "iamspy");

            // 2. get system clock and wrap it with a fast-forward adapter
            const systemClock = getSystemClock(player);
            const fastForwardClock = new FastForwardClock(systemClock, clock, 50);

            // 3. get scout-1 and its state
            const scout1 = getShip(player, "scout-1");
            const scout1State = expectOk(await scout1.get_state(), "scout-1 state");
            expect(scout1State.weight, "scout-1 weight").toBeDefined();
            if (scout1State.weight === undefined) {
                return;
            }
            const shipSpec = expectOk(
                await scout1.get_specification(),
                "scout-1 specification",
            );

            // 4. get scout-2
            const scout2 = getShip(player, "scout-2");

            // 5. get scout-1 hover engine and specification
            const engine = getHoverEngine(scout1, "engine");
            const engineSpec = expectOk(
                await engine.get_specification(),
                "engine specification",
            );

            // 6. get both ships' positions and build an intercept plan
            const [timeStatus, now] = await systemClock.time();
            expectStatus(timeStatus, "current time");
            if (now === undefined) {
                return;
            }
            const position = expectOk(
                await scout1.get_position(now + 1_000_000),
                "scout-1 position",
            );
            const target = expectOk(await scout2.get_position(), "scout-2 position");
            const flightPlan = build_plan(
                {
                    mass: scout1State.weight,
                    position,
                    orientation: [
                        scout1State.orientation[0],
                        scout1State.orientation[1],
                    ],
                    engine_max_thrust: engineSpec.max_thrust,
                    max_rotate_speed: shipSpec.max_rotation_speed,
                },
                target,
            );

            // 7. follow the flight plan
            expectStatus(
                await follow_flight_plan(
                    scout1.down_level("ship"),
                    engine.down_level(),
                    flightPlan,
                    fastForwardClock,
                    shipSpec.max_rotation_speed,
                ),
                "follow flight plan",
            );
        });
    },
);

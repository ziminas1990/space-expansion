import { expect, test } from "vitest";
import {
    approach_to_plan,
    follow_flight_plan,
} from "../highlevel/index.js";
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
    getEngine,
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

            // 4. get scout-2
            const scout2 = getShip(player, "scout-2");

            // 5. get scout-1 engine and specification
            const engine = getEngine(scout1, "main_engine");
            const engineSpec = expectOk(
                await engine.get_specification(),
                "engine specification",
            );

            // 6. get both ships' positions and build an intercept plan
            const position = expectOk(await scout1.get_position(), "scout-1 position");
            const target = expectOk(await scout2.get_position(), "scout-2 position");
            const amax = engineSpec.max_thrust / scout1State.weight;
            const flightPlan = approach_to_plan(position, target, amax);
            expect(flightPlan, "intercept plan").toBeTruthy();
            if (!flightPlan) {
                return;
            }

            // 7. follow the flight plan
            expectStatus(
                await follow_flight_plan(
                    scout1,
                    engine,
                    flightPlan,
                    fastForwardClock,
                ),
                "follow flight plan",
            );
        });
    },
);

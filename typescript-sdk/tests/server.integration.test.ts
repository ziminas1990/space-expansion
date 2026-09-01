import { expect, test } from "vitest";
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

test.skipIf(!hasServerBinary)(
    "starts a configured server and opens the administrator interface",
    { timeout: integrationTimeoutMs },
    async () => {
        const configuration = new Configuration({
            general: new General({
                totalThreads: 1,
                loginUdpPort: 7_456,
                initialState: ApplicationMode.Freeze,
                portsPool: [12_000, 12_100],
            }),
            blueprints: new DefaultBlueprints(),
            world: new World(),
        });

        await withServer(configuration, async ({
            administrator,
            clock,
            server,
        }) => {
            // 1. check server is running and frozen time is zero
            expect(server.isRunning()).toBe(true);
            expect(await clock.time()).toBe(0n);

            // 2. proceed frozen time by 2000ms
            const after = await clock.proceed(2_000, 1_000);

            // 3. check time advanced by ~2000000us
            expect(after).toBeGreaterThanOrEqual(1_999_000n);
            expect(after).toBeLessThanOrEqual(2_001_000n);

            // 4. check administrator clock, spawner, and manipulator
            const [timeStatus, time] = await administrator.clock.get_time();
            expect(timeStatus.is_ok()).toBe(true);
            expect(typeof time).toBe("bigint");
            expect(administrator.spawner).toBeTruthy();
            expect(administrator.manipulator).toBeTruthy();
        });
    },
);

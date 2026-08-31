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
            expect(server.isRunning()).toBe(true);
            expect(await clock.time()).toBe(0n);
            const after = await clock.proceed(2_000, 1_000);
            expect(after).toBeGreaterThanOrEqual(1_999_000n);
            expect(after).toBeLessThanOrEqual(2_001_000n);
            const [timeStatus, time] = await administrator.clock.get_time();
            expect(timeStatus.is_ok()).toBe(true);
            expect(typeof time).toBe("bigint");
            expect(administrator.spawner).toBeTruthy();
            expect(administrator.manipulator).toBeTruthy();
        });
    },
);

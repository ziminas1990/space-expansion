import { expect, test } from "vitest";
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
import { expectOk, expectStatus, getSystemClock } from "./helpers/index.js";

function systemClockConfiguration(): Configuration {
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

function sleep(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

function track<T>(promise: Promise<T>): { done: boolean; promise: Promise<T> } {
    const tracked = { done: false, promise };
    void promise.finally(() => {
        tracked.done = true;
    });
    return tracked;
}

function expectCloseUs(
    actual: bigint,
    expected: bigint,
    slackUs = 5_000n,
    description = "time",
): void {
    const delta = actual >= expected ? actual - expected : expected - actual;
    expect(delta, `${description}: delta ${delta}us`).toBeLessThanOrEqual(slackUs);
}

test.skipIf(!hasServerBinary)(
    "syncs time and completes wait_until and wait_for",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(systemClockConfiguration(), async ({ login, clock }) => {
            const player = await login("player", "player");
            const systemClock = getSystemClock(player);

            const stoppedAt = await clock.stop();
            const reportedAt = expectOk(
                await systemClock.down_level().get_time(),
                "time_req",
            );
            expect(reportedAt.ingame_us).toBe(stoppedAt);
            expectCloseUs(
                expectOk(await systemClock.time(false), "system clock time"),
                stoppedAt,
                5_000n,
                "highlevel time after stop",
            );

            const currentTimePoint = systemClock.time_point();
            const proceededTo = await clock.proceed(1_000, 1_000);
            expectStatus(await systemClock.sync(), "sync system clock");
            expectCloseUs(
                currentTimePoint.us(),
                proceededTo,
                5_000n,
                "time_point after sync",
            );
            expectCloseUs(
                expectOk(await systemClock.time(false), "time after sync"),
                proceededTo,
                5_000n,
                "highlevel time after sync",
            );

            const waitDeltaMs = 10_000;
            const waitUntil = track(
                systemClock.wait_until(
                    proceededTo + BigInt(waitDeltaMs) * 1_000n,
                    1_000,
                ),
            );

            await clock.proceed(waitDeltaMs - 1, 1_000);
            await sleep(10);
            expect(waitUntil.done).toBe(false);

            await clock.proceed(1, 1_000);
            await sleep(10);
            expect(waitUntil.done).toBe(true);
            expectOk(await waitUntil.promise, "wait_until");

            const waitFor = track(
                systemClock.wait_for(BigInt(waitDeltaMs) * 1_000n, 1_000),
            );

            await clock.proceed(waitDeltaMs - 1, 1_000);
            await sleep(10);
            expect(waitFor.done).toBe(false);

            await clock.proceed(1, 1_000);
            await sleep(10);
            expect(waitFor.done).toBe(true);
            expectOk(await waitFor.promise, "wait_for");
        });
    },
);

test.skipIf(!hasServerBinary)(
    "completes overlapping wait_for and wait_until sessions",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(systemClockConfiguration(), async ({ login, clock }) => {
            const player = await login("player", "player");
            const systemClock = getSystemClock(player);

            const stoppedAt = await clock.stop();

            const waitFor2s = track(systemClock.wait_for(2_000_000n, 1_000));
            const waitUntil10s = track(
                systemClock.wait_until(stoppedAt + 10_000_000n, 1_000),
            );
            const waitUntil5s = track(
                systemClock.wait_until(stoppedAt + 5_000_000n, 1_000),
            );
            const waitFor50s = track(systemClock.wait_for(50_000_000n, 1_000));
            await sleep(10);

            await clock.proceed(2_000, 1_000);
            await sleep(10);
            expect(waitFor2s.done).toBe(true);
            expectOk(await waitFor2s.promise, "wait_for 2s");

            const waitFor3s = track(systemClock.wait_for(3_000_000n, 1_000));

            await clock.proceed(3_000, 1_000);
            await sleep(100);
            expect(waitUntil5s.done).toBe(true);
            expect(waitFor3s.done).toBe(true);
            expectOk(await waitUntil5s.promise, "wait_until 5s");
            expectOk(await waitFor3s.promise, "wait_for 3s");

            await clock.proceed(5_000, 1_000);
            await sleep(10);
            expect(waitUntil10s.done).toBe(true);
            expectOk(await waitUntil10s.promise, "wait_until 10s");

            await clock.proceed(46_000, 5_000);
            await sleep(10);
            expect(waitFor50s.done).toBe(true);
            expectOk(await waitFor50s.promise, "wait_for 50s");
        });
    },
);

test.skipIf(!hasServerBinary)(
    "monitors timestamps at the requested intervals",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(systemClockConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(10);

            const player = await login("player", "player");
            const systemClock = getSystemClock(player);

            const sessions = [
                { interval: 110, timestamps: [] as bigint[] },
                { interval: 75, timestamps: [] as bigint[] },
                { interval: 55, timestamps: [] as bigint[] },
                { interval: 20, timestamps: [] as bigint[] },
            ];
            const stop = { value: false };

            await clock.stop();
            const monitoring = sessions.map((session) =>
                systemClock.down_level().monitoring(session.interval, async (timestamp) => {
                    if (timestamp) {
                        session.timestamps.push(timestamp.ingame_us);
                    }
                    return !stop.value;
                })
            );

            await clock.fastForward(5);
            expectOk(await systemClock.wait_for(10_000_000n, 5_000), "wait 10s");
            const endAt = await clock.stop();

            stop.value = true;
            for (const status of await Promise.all(monitoring)) {
                expectStatus(status, "clock monitoring");
            }

            for (const session of sessions) {
                expect(
                    session.timestamps.length,
                    `interval ${session.interval}ms produced no timestamps`,
                ).toBeGreaterThan(0);

                const sessionDuration =
                    Number(endAt - session.timestamps[0]!) / 1_000;
                const totalExpected =
                    1 + Math.floor(sessionDuration / session.interval);
                expect(
                    session.timestamps.length,
                    `interval ${session.interval}ms timestamp count`,
                ).toBe(totalExpected);

                for (let i = 1; i < session.timestamps.length; i += 1) {
                    expect(
                        session.timestamps[i]! - session.timestamps[i - 1]!,
                        `interval ${session.interval}ms step ${i}`,
                    ).toBe(BigInt(session.interval * 1_000));
                }
            }
        });
    },
);

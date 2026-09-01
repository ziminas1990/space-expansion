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
            // 1. player logins
            const player = await login("player", "player");

            // 2. get system clock
            const systemClock = getSystemClock(player);

            // 3. stop administrator clock and check times match
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

            // 4. proceed time and sync the system clock
            const currentTimePoint = systemClock.time_point();
            const proceededTo = await clock.proceed(1_000, 1_000);
            expectStatus(await systemClock.sync(), "sync system clock");

            // 5. check time_point and time after sync
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

            // 6. start wait_until
            const waitDeltaMs = 10_000;
            const waitUntil = track(
                systemClock.wait_until(
                    proceededTo + BigInt(waitDeltaMs) * 1_000n,
                    1_000,
                ),
            );

            // 7. proceed almost to the deadline and check wait is still pending
            await clock.proceed(waitDeltaMs - 1, 1_000);
            await sleep(10);
            expect(waitUntil.done).toBe(false);

            // 8. proceed the last millisecond and check wait_until completes
            await clock.proceed(1, 1_000);
            await sleep(10);
            expect(waitUntil.done).toBe(true);
            expectOk(await waitUntil.promise, "wait_until");

            // 9. start wait_for
            const waitFor = track(
                systemClock.wait_for(BigInt(waitDeltaMs) * 1_000n, 1_000),
            );

            // 10. proceed almost to the deadline and check wait is still pending
            await clock.proceed(waitDeltaMs - 1, 1_000);
            await sleep(10);
            expect(waitFor.done).toBe(false);

            // 11. proceed the last millisecond and check wait_for completes
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
            // 1. player logins
            const player = await login("player", "player");

            // 2. get system clock
            const systemClock = getSystemClock(player);

            // 3. stop administrator clock
            const stoppedAt = await clock.stop();

            // 4. start overlapping wait_for and wait_until sessions
            const waitFor2s = track(systemClock.wait_for(2_000_000n, 1_000));
            const waitUntil10s = track(
                systemClock.wait_until(stoppedAt + 10_000_000n, 1_000),
            );
            const waitUntil5s = track(
                systemClock.wait_until(stoppedAt + 5_000_000n, 1_000),
            );
            const waitFor50s = track(systemClock.wait_for(50_000_000n, 1_000));
            await sleep(10);

            // 5. proceed 2s and check wait_for 2s completes
            await clock.proceed(2_000, 1_000);
            await sleep(10);
            expect(waitFor2s.done).toBe(true);
            expectOk(await waitFor2s.promise, "wait_for 2s");

            // 6. start wait_for 3s
            const waitFor3s = track(systemClock.wait_for(3_000_000n, 1_000));

            // 7. proceed 3s and check wait_until 5s and wait_for 3s complete
            await clock.proceed(3_000, 1_000);
            await sleep(100);
            expect(waitUntil5s.done).toBe(true);
            expect(waitFor3s.done).toBe(true);
            expectOk(await waitUntil5s.promise, "wait_until 5s");
            expectOk(await waitFor3s.promise, "wait_for 3s");

            // 8. proceed 5s and check wait_until 10s completes
            await clock.proceed(5_000, 1_000);
            await sleep(10);
            expect(waitUntil10s.done).toBe(true);
            expectOk(await waitUntil10s.promise, "wait_until 10s");

            // 9. proceed 46s and check wait_for 50s completes
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

            // 1. player logins
            const player = await login("player", "player");

            // 2. get system clock
            const systemClock = getSystemClock(player);

            // 3. start monitoring sessions at different intervals
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

            // 4. wait 10s of ingame time
            await clock.fastForward(5);
            expectOk(await systemClock.wait_for(10_000_000n, 5_000), "wait 10s");
            const endAt = await clock.stop();

            // 5. stop monitoring
            stop.value = true;
            for (const status of await Promise.all(monitoring)) {
                expectStatus(status, "clock monitoring");
            }

            // 6. check timestamps for each session
            for (const session of sessions) {
                // 6.1 check timestamps were produced
                expect(
                    session.timestamps.length,
                    `interval ${session.interval}ms produced no timestamps`,
                ).toBeGreaterThan(0);

                // 6.2 check timestamp count matches the interval
                const sessionDuration =
                    Number(endAt - session.timestamps[0]!) / 1_000;
                const totalExpected =
                    1 + Math.floor(sessionDuration / session.interval);
                expect(
                    session.timestamps.length,
                    `interval ${session.interval}ms timestamp count`,
                ).toBe(totalExpected);

                // 6.3 check steps equal the interval
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

import { expect, test } from "vitest";
import type { CurrentThrust, RCS } from "../highlevel/index.js";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    General,
    makeProbe,
    Player,
    Position,
    World,
} from "./configurator/index.js";
import {
    hasServerBinary,
    integrationTimeoutMs,
    withServer,
} from "./fixture.js";
import {
    collectEvent,
    Collector,
    expectOk,
    expectStatus,
    getRCS,
    getShip,
} from "./helpers/index.js";

function engineConfiguration(): Configuration {
    return new Configuration({
        general: new General({
            totalThreads: 1,
            loginUdpPort: 7_456,
            initialState: ApplicationMode.Freeze,
            portsPool: [12_000, 12_100],
        }),
        blueprints: new DefaultBlueprints(),
        world: new World(),
        players: [
            new Player({
                login: "player",
                password: "player",
                ships: [makeProbe("scout-1", new Position(0, 0))],
            }),
        ],
    });
}

function expectThrust(
    actual: CurrentThrust,
    x: number,
    y: number,
    thrust: number,
): void {
    expect(actual.thrust).toBe(thrust);
    expect(actual.x).toBeCloseTo(x, 5);
    expect(actual.y).toBeCloseTo(y, 5);
}

async function waitJournal(
    clock: { proceed(proceedMs: number, timeoutMs: number): Promise<number> },
    journal: Collector<CurrentThrust>,
    count: number,
    description: string,
): Promise<void> {
    const deadline = Date.now() + 2_000;
    while (journal.length < count && Date.now() < deadline) {
        await clock.proceed(50, 2_000);
        await new Promise((resolve) => setTimeout(resolve, 20));
    }
    await journal.waitForCount(count, description);
}

async function monitorThrust(
    engine: RCS,
    journal: Collector<CurrentThrust>,
    stopped: { value: boolean },
): Promise<void> {
    expectStatus(
        await engine.down_level().monitoring(async (thrust) => {
            if (thrust) {
                journal.push(thrust);
            }
            return !stopped.value;
        }),
        "engine monitoring",
    );
}

test.skipIf(!hasServerBinary)(
    "monitors maximum thrust along the commanded direction",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(engineConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("player", "player");

            // 2. get engine specification
            const rcs = getRCS(getShip(player, "scout-1"), "main_rcs");
            const spec = expectOk(
                await rcs.get_specification(),
                "engine specification",
            );
            expect(spec.max_thrust).toBe(1_000);
            await clock.stop();

            // 3. start monitoring
            const journal = new Collector<CurrentThrust>();
            const stopped = { value: false };
            const monitoring = monitorThrust(rcs, journal, stopped);

            try {
                // 4. wait for the snapshot
                await waitJournal(clock, journal, 1, "engine thrust snapshot");
                expectThrust(journal.items[0]!, 0, 0, 0);

                // 5. a direction applies maximum thrust along that direction
                expectStatus(
                    await rcs.set_thrust(3, 4, 1_000_000),
                    "set direction",
                );
                await waitJournal(clock, journal, 2, "applied thrust");
                expectThrust(journal.items[1]!, 600, 800, spec.max_thrust);

                // 6. a shorter vector in the same direction does not change the thrust
                expectStatus(
                    await rcs.set_thrust(0.6, 0.8, 1_000_000),
                    "shorter vector",
                );
                await waitJournal(clock, journal, 3, "shorter vector applied");
                expectThrust(journal.items[2]!, 600, 800, spec.max_thrust);

                // 7. a longer vector in the same direction does not change the thrust
                expectStatus(
                    await rcs.set_thrust(6, 8, 1_000_000),
                    "longer vector",
                );
                await waitJournal(clock, journal, 4, "longer vector applied");
                expectThrust(journal.items[3]!, 600, 800, spec.max_thrust);

                // 8. a zero vector produces no thrust
                expectStatus(await rcs.set_thrust(0, 0, 1_000_000), "zero vector");
                await waitJournal(clock, journal, 5, "zero thrust");
                expectThrust(journal.items[4]!, 0, 0, 0);
            } finally {
                stopped.value = true;
                await monitoring;
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "monitors duration expiry and delayed change_thrust",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(engineConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("player", "player");
            const rcs = getRCS(getShip(player, "scout-1"), "main_rcs");
            const spec = expectOk(
                await rcs.get_specification(),
                "engine specification",
            );
            await clock.stop();

            // 2. start monitoring
            const journal = new Collector<CurrentThrust>();
            const stopped = { value: false };
            const monitoring = monitorThrust(rcs, journal, stopped);

            try {
                // 3. wait for the snapshot
                await waitJournal(clock, journal, 1, "engine thrust snapshot");
                expectThrust(journal.items[0]!, 0, 0, 0);

                // 4. start a short burn
                expectStatus(await rcs.set_thrust(1, 0, 300), "start burn");
                await waitJournal(clock, journal, 2, "burn applied");
                expectThrust(journal.items[1]!, spec.max_thrust, 0, spec.max_thrust);

                // 5. when duration elapses, monitor receives zero thrust
                await waitJournal(clock, journal, 3, "burn expired");
                expectThrust(journal.items[2]!, 0, 0, 0);

                // 6. schedule a delayed change_thrust
                const now = await clock.time();
                expectStatus(
                    await rcs.set_thrust(0, 1, 1_000_000, now + 200_000),
                    "delayed thrust",
                );
                await clock.proceed(50, 2_000);
                await new Promise((resolve) => setTimeout(resolve, 150));
                expect(journal.length).toBe(3);

                // 7. after the timestamp, monitor receives the applied vector
                await waitJournal(clock, journal, 4, "delayed thrust applied");
                expectThrust(
                    journal.items[3]!,
                    0,
                    spec.max_thrust,
                    spec.max_thrust,
                );
            } finally {
                stopped.value = true;
                await monitoring;
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "emits highlevel thrust events",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(engineConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("player", "player");
            const rcs = getRCS(getShip(player, "scout-1"), "main_rcs");
            const spec = expectOk(
                await rcs.get_specification(),
                "engine specification",
            );

            // 2. subscribe to highlevel thrust events
            const events = collectEvent(rcs, "thrust");

            // 3. apply a new thrust direction
            expectStatus(await rcs.set_thrust(0, 1, 10_000), "set thrust");

            // 4. the engine emits maximum thrust along that direction
            await events.waitFor(
                (items) => items.some((item) => item.thrust === spec.max_thrust),
                "applied thrust event",
            );
            expectThrust(
                events.items.find((item) => item.thrust === spec.max_thrust)!,
                0,
                spec.max_thrust,
                spec.max_thrust,
            );
        });
    },
);

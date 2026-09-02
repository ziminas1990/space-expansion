import { test } from "vitest";
import type { Player as SdkPlayer, SystemClock } from "../highlevel/index.js";
import type {
    Messanger as MessangerRpc,
    MessangerService,
} from "../midlevel/index.js";
import { Status } from "../types/status.js";
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
    expectOk,
    expectStatus,
    getMessanger,
    getSystemClock,
    Randomizer,
} from "./helpers/index.js";

const SERVICES_LIMIT = 32;
const SERVICES_TOTAL = 3;
const ASCII_LETTERS =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const WAIT_REQUEST_TIMEOUT_MS = 100;
const ACTIVE_REQUEST_TIMEOUT_MS = 3_000;
const INACTIVE_REQUEST_TIMEOUT_MS = 500;
const CLOCK_WAIT_TIMEOUT_MS = 5_000;
const SERVICE_WAIT_UNTIL_TIMEOUT_MS = 60_000;

const Dice = {
    SPAWN_SERVICE: 1,
    CHECK_SERVICES_LIST: 5,
    REQUEST_TO_INACTIVE_SERVICE: 10,
    SEND_REQUEST: 100,
} as const;

type DiceAction = keyof typeof Dice;

const DICE_ACTIONS = Object.keys(Dice) as DiceAction[];
const DICE_WEIGHTS = DICE_ACTIONS.map((action) => Dice[action]);

function messangerConfiguration(): Configuration {
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
                password: "awesome",
                ships: [],
            }),
        ],
    });
}

function sleep(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

function throwDice(rng: Randomizer): DiceAction {
    return rng.choiceWeighted(DICE_ACTIONS, DICE_WEIGHTS);
}

function randomLetters(rng: Randomizer, length: number): string {
    let body = "";
    for (let i = 0; i < length; i += 1) {
        body += ASCII_LETTERS[rng.randomInt(0, ASCII_LETTERS.length - 1)];
    }
    return body;
}

function isStatus(status: Status, code: string): boolean {
    return status.what() === code;
}

class Interval {
    constructor(
        readonly begin: bigint,
        readonly end: bigint,
    ) {}

    contains(point: bigint): boolean {
        return this.begin <= point && point <= this.end;
    }

    withMargin(margin: number): Interval {
        if (!(0 < 2 * margin && 2 * margin < 1)) {
            throw new Error("margin must satisfy 0 < 2 * margin < 1");
        }
        const marginUs = BigInt(
            Math.round(Number(this.end - this.begin) * margin),
        );
        return new Interval(this.begin + marginUs, this.end - marginUs);
    }

    cutByNow(now: bigint): Interval {
        return new Interval(
            now > this.begin ? now : this.begin,
            now > this.end ? now : this.end,
        );
    }

    randomSubInterval(rng: Randomizer, lengthPart: number): Interval {
        if (!(0 < lengthPart && lengthPart < 1)) {
            throw new Error("lengthPart must satisfy 0 < lengthPart < 1");
        }
        const frameLength = Number(this.end - this.begin);
        const length = Math.round(frameLength * lengthPart);
        const leftOffset = rng.randomInt(0, frameLength - length);
        return new Interval(
            this.begin + BigInt(leftOffset),
            this.begin + BigInt(leftOffset + length),
        );
    }
}

type MessengerSession = {
    player: SdkPlayer;
    messanger: MessangerRpc;
    clock: SystemClock;
};

async function newSession(
    login: (login: string, password: string) => Promise<SdkPlayer>,
): Promise<MessengerSession> {
    const player = await login("player", "awesome");
    return {
        player,
        messanger: getMessanger(player).down_level(),
        clock: getSystemClock(player),
    };
}

class DuplicatingService {
    service: MessangerService | undefined;
    registered = false;
    private task: Promise<void> | undefined;
    private finished = false;

    constructor(
        readonly session: MessengerSession,
        readonly name: string,
        readonly multiplier: number,
        readonly lifetime: Interval,
    ) {}

    run(): void {
        if (this.task === undefined) {
            this.task = this.impl().finally(() => {
                this.finished = true;
            });
        }
    }

    done(): boolean {
        return this.task === undefined || this.finished;
    }

    join(): Promise<void> {
        return this.task ?? Promise.resolve();
    }

    expectedResponse(request: string): string {
        return request.repeat(this.multiplier);
    }

    isActive(now: bigint, margin = 0.05): boolean {
        return this.lifetime.withMargin(margin).contains(now) && this.registered;
    }

    isInactive(now: bigint): boolean {
        return !this.lifetime.contains(now) && !this.registered;
    }

    private async impl(): Promise<void> {
        await this.session.clock.wait_until(
            this.lifetime.begin,
            SERVICE_WAIT_UNTIL_TIMEOUT_MS,
        );

        const [status, service] = await this.session.messanger.open_service(
            this.name,
        );
        if (!status.is_ok() || service === undefined) {
            return;
        }
        this.service = service;
        this.registered = true;

        try {
            service.drop_queued_requests();
            let [timeStatus, now] = await this.session.clock.time();
            if (!timeStatus.is_ok() || now === undefined) {
                return;
            }

            while (now < this.lifetime.end) {
                const [waitStatus, request] = await service.wait_request(
                    WAIT_REQUEST_TIMEOUT_MS,
                );
                if (waitStatus.is_closed()) {
                    break;
                }
                if (waitStatus.is_timeout()) {
                    // no requests for now
                } else if (request !== undefined) {
                    await service.send_response(
                        request,
                        this.expectedResponse(request.body),
                    );
                }
                [timeStatus, now] = await this.session.clock.time();
                if (!timeStatus.is_ok() || now === undefined) {
                    break;
                }
            }
        } finally {
            await service.close();
            this.registered = false;
        }
    }
}

class Environment {
    readonly services: DuplicatingService[] = [];
    private nextServiceId = 1;

    constructor(
        readonly lifetime: Interval,
        private rng: Randomizer,
    ) {}

    getActiveServices(now: bigint): DuplicatingService[] {
        return this.services.filter((service) => service.isActive(now));
    }

    getNonActiveServices(now: bigint): DuplicatingService[] {
        return this.services.filter(
            (service) => service.isInactive(now) && service.done(),
        );
    }

    spawnRandomService(
        session: MessengerSession,
        lifetime: Interval,
    ): [Status, DuplicatingService] {
        const serviceName = `service_${this.nextServiceId}`;
        this.nextServiceId += 1;

        const duplicatingService = new DuplicatingService(
            session,
            serviceName,
            this.rng.randomInt(2, 20),
            lifetime,
        );
        duplicatingService.run();
        this.services.push(duplicatingService);
        return [Status.ok(), duplicatingService];
    }

    async join(): Promise<void> {
        await Promise.all(this.services.map((service) => service.join()));
    }

    checkIsDone(): Status {
        for (const service of this.services) {
            if (!service.done()) {
                return Status.fail(`Service ${service.name} is still active`);
            }
        }
        return Status.ok();
    }
}

class StressPlayer {
    constructor(
        readonly session: MessengerSession,
        readonly env: Environment,
        readonly lifetime: Interval,
        private rng: Randomizer,
    ) {}

    async doSomething(): Promise<Status> {
        const now = expectOk(await this.session.clock.time(), "player clock");
        if (!this.lifetime.contains(now)) {
            return Status.ok();
        }

        const action = throwDice(this.rng);
        if (action === "SEND_REQUEST") {
            return this.sendRandomRequest(now);
        }
        if (action === "SPAWN_SERVICE") {
            const [status] = this.env.spawnRandomService(
                this.session,
                this.lifetime
                    .cutByNow(now)
                    .withMargin(0.05)
                    .randomSubInterval(
                        this.rng,
                        0.3 + 0.5 * this.rng.randomValue(0, 1),
                    ),
            );
            if (isStatus(status, "TOO_MANY_SERVCES")) {
                if (this.env.getActiveServices(now).length === SERVICES_LIMIT) {
                    return Status.ok();
                }
            }
            return status;
        }
        if (action === "REQUEST_TO_INACTIVE_SERVICE") {
            return this.sendRandomRequestToInactiveService(now);
        }
        if (action === "CHECK_SERVICES_LIST") {
            return this.checkServicesList();
        }
        return Status.ok();
    }

    private async sendRandomRequest(now: bigint): Promise<Status> {
        const candidates = this.env.getActiveServices(now);
        if (candidates.length === 0) {
            return Status.ok();
        }
        const service = this.rng.choice(candidates);
        const request = randomLetters(this.rng, 8);
        const [status, response] = await this.session.messanger.send_request(
            service.name,
            request,
            ACTIVE_REQUEST_TIMEOUT_MS,
        );
        if (!status.is_ok()) {
            return status.wrap(`failed to send request to ${service.name}`);
        }
        const expected = service.expectedResponse(request);
        if (response !== expected) {
            return Status.fail(
                `Got unexpected response: '${response}'. Expected: ${expected}`,
            );
        }
        return Status.ok();
    }

    private async sendRandomRequestToInactiveService(
        now: bigint,
    ): Promise<Status> {
        const candidates = this.env.getNonActiveServices(now);
        if (candidates.length === 0) {
            return Status.ok();
        }
        const service = this.rng.choice(candidates);
        const [status] = await this.session.messanger.send_request(
            service.name,
            "test",
            INACTIVE_REQUEST_TIMEOUT_MS,
        );
        if (isStatus(status, "NO_SUCH_SERVICE")) {
            return Status.ok();
        }
        return Status.fail(
            `Got a response from inactive service ${service.name}: ${status.what()}`,
        );
    }

    private async checkServicesList(): Promise<Status> {
        const [status, listed] = await this.session.messanger.services_list();
        if (!status.is_ok() || listed === undefined) {
            return status.wrap("Can't get services list");
        }
        for (const expected of this.env.getActiveServices(listed.timestamp)) {
            if (!listed.services.includes(expected.name)) {
                return Status.fail(
                    `Services list mismatch! Service ${expected.name} not listed`,
                );
            }
        }
        return Status.ok();
    }
}

test.skipIf(!hasServerBinary)(
    "single player sends random requests against short-lived duplicating services",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(messangerConfiguration(), async ({ login, clock }) => {
            const rng = new Randomizer(9_017);

            // 1. fast-forward the administrator clock
            await clock.fastForward(20);

            // 2. player logins a main session
            const mainSession = await newSession(login);

            // 3. pick a two-minute ingame lifetime
            const startedAt = expectOk(
                await mainSession.clock.time(),
                "main session time",
            );
            const lifetime = new Interval(
                startedAt + 10_000_000n,
                startedAt + 130_000_000n,
            );

            // 4. spawn a number of services
            const environment = new Environment(lifetime, rng);
            for (let i = 0; i < SERVICES_TOTAL; i += 1) {
                const [status] = environment.spawnRandomService(
                    mainSession,
                    environment.lifetime.withMargin(0.05),
                );
                expectStatus(status, `spawn service ${i}`);
            }

            // 5. player logins a second session
            const player = new StressPlayer(
                await newSession(login),
                environment,
                environment.lifetime.withMargin(0.01),
                rng,
            );

            // 6. do some random thing each ~200ms
            let now = expectOk(
                await mainSession.clock.time(),
                "main session time",
            );
            while (now < lifetime.end) {
                // 6.1 player does something
                expectStatus(await player.doSomething(), "player action");

                // 6.2 wait 200ms of ingame time
                now = expectOk(
                    await mainSession.clock.wait_for(
                        200_000n,
                        CLOCK_WAIT_TIMEOUT_MS,
                    ),
                    "wait 200ms",
                );
            }

            // 7. give services some time to finish
            await sleep(200);
            await environment.join();
            expectStatus(environment.checkIsDone(), "environment done");
        });
    },
);

test.skipIf(!hasServerBinary)(
    "15 sessions send random requests against short-lived duplicating services",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(messangerConfiguration(), async ({ login, clock }) => {
            const rng = new Randomizer(9_018);

            // 1. fast-forward the administrator clock
            await clock.fastForward(20);

            // 2. player logins a main session
            const mainSession = await newSession(login);

            // 3. pick a two-minute ingame lifetime
            const startedAt = expectOk(
                await mainSession.clock.time(),
                "main session time",
            );
            const lifetime = new Interval(
                startedAt + 10_000_000n,
                startedAt + 130_000_000n,
            );

            // 4. spawn a number of services
            const environment = new Environment(lifetime, rng);
            for (let i = 0; i < SERVICES_TOTAL; i += 1) {
                const [status] = environment.spawnRandomService(
                    mainSession,
                    environment.lifetime.withMargin(0.05),
                );
                expectStatus(status, `spawn service ${i}`);
            }

            // 5. login 15 sessions for the same player
            const players: StressPlayer[] = [];
            for (let i = 0; i < 15; i += 1) {
                players.push(
                    new StressPlayer(
                        await newSession(login),
                        environment,
                        environment.lifetime
                            .withMargin(0.01)
                            .randomSubInterval(
                                rng,
                                0.5 + 0.4 * rng.randomValue(0, 1),
                            ),
                        rng,
                    ),
                );
            }

            // 6. do some random thing each ~100ms
            let now = expectOk(
                await mainSession.clock.time(),
                "main session time",
            );
            while (now < lifetime.end) {
                // 6.1 each player does something
                for (const player of players) {
                    expectStatus(await player.doSomething(), "player action");
                }

                // 6.2 wait 100ms of ingame time
                now = expectOk(
                    await mainSession.clock.wait_for(
                        100_000n,
                        CLOCK_WAIT_TIMEOUT_MS,
                    ),
                    "wait 100ms",
                );
            }

            // 7. give services some time to finish
            await sleep(200);
            await environment.join();
            expectStatus(environment.checkIsDone(), "environment done");
        });
    },
);

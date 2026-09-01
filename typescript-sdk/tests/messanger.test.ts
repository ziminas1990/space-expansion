import { expect, test } from "vitest";
import type { MessangerService } from "../midlevel/index.js";
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
    getMessanger,
    Randomizer,
} from "./helpers/index.js";

const SERVICES_LIMIT = 32;
const MAX_REQUEST_TIMEOUT_MS = 5_000;

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

function expectError(
    status: { is_ok(): boolean; what(): string },
    code: string,
): void {
    expect(status.what()).toBe(code);
}

function reverseBody(body: string): string {
    return [...body].reverse().join("");
}

function duplicateBody(body: string): string {
    return body + body;
}

async function runTransformService(
    service: MessangerService,
    transform: (body: string) => string,
): Promise<void> {
    let [status, request] = await service.wait_request();
    while (status.is_ok() && request) {
        await service.send_response(request, transform(request.body));
        [status, request] = await service.wait_request();
        if (status.is_closed()) {
            return;
        }
    }
}

class ReverseService {
    constructor(private service: MessangerService) {}

    async run(): Promise<void> {
        this.service.drop_queued_requests();
        return runTransformService(this.service, reverseBody);
    }
}

class DuplicateService {
    constructor(private service: MessangerService) {}

    async run(): Promise<void> {
        return runTransformService(this.service, duplicateBody);
    }
}

test.skipIf(!hasServerBinary)(
    "opens a service",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(messangerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            const player = await login("player", "awesome");
            const messanger = getMessanger(player).down_level();

            const service = expectOk(
                await messanger.open_service("awesomesvc"),
                "open service",
            );
            expect(service).toBeTruthy();
        });
    },
);

test.skipIf(!hasServerBinary)(
    "rejects a duplicate service name and enforces the services limit",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(messangerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            const player = await login("player", "awesome");
            const messanger = getMessanger(player).down_level();

            const service = expectOk(
                await messanger.open_service("awesomesvc"),
                "open service",
            );

            const [duplicateStatus, duplicate] = await messanger.open_service(
                "awesomesvc",
            );
            expectError(duplicateStatus, "SERVICE_EXISTS");
            expect(duplicate).toBeUndefined();
            await service.close();

            for (let i = 0; i < SERVICES_LIMIT; i += 1) {
                expectOk(
                    await messanger.open_service(`svc_${i}`),
                    `open service svc_${i}`,
                );
            }

            const [overflowStatus, overflow] = await messanger.open_service(
                "yet_another_service",
            );
            expectError(overflowStatus, "TOO_MANY_SERVCES");
            expect(overflow).toBeUndefined();
        });
    },
);

test.skipIf(!hasServerBinary)(
    "lists services as they are opened and closed",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(messangerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            const player = await login("player", "awesome");
            const messanger = getMessanger(player).down_level();
            const randomizer = new Randomizer(1);

            const allNames = randomizer.shuffle(
                Array.from({ length: SERVICES_LIMIT }, (_, i) => `service_${i}`),
            );
            const services: MessangerService[] = [];

            for (const serviceName of allNames) {
                services.push(
                    expectOk(
                        await messanger.open_service(serviceName),
                        `open service ${serviceName}`,
                    ),
                );

                const listed = expectOk(
                    await messanger.services_list(),
                    "services list after open",
                );
                expect([...listed.services].sort()).toEqual(
                    services.map((service) => service.name).sort(),
                );
            }

            randomizer.shuffle(services);
            while (services.length > 0) {
                const service = services.pop()!;
                await service.close();

                const listed = expectOk(
                    await messanger.services_list(),
                    "services list after close",
                );
                expect([...listed.services].sort()).toEqual(
                    services.map((entry) => entry.name).sort(),
                );
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "rejects requests to a missing service, overlong timeouts, and idle services",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(messangerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            const player = await login("player", "awesome");
            const messanger = getMessanger(player).down_level();

            const [missingStatus] = await messanger.send_request(
                "wrongsvc",
                "ping",
            );
            expectError(missingStatus, "NO_SUCH_SERVICE");

            const serviceName = "awesomesvc";
            expectOk(
                await messanger.open_service(serviceName),
                "open service",
            );

            const [tooLongStatus] = await messanger.send_request(
                serviceName,
                "ping",
                MAX_REQUEST_TIMEOUT_MS + 10,
            );
            expectError(tooLongStatus, "REQUEST_TIMEOUT_TOO_LONG");

            const [closedStatus] = await messanger.send_request(
                serviceName,
                "ping",
                2_000,
            );
            expectError(closedStatus, "CLOSED");
        });
    },
);

test.skipIf(!hasServerBinary)(
    "reverses a request body through a hosted service",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(messangerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            const player = await login("player", "awesome");
            const messanger = getMessanger(player).down_level();

            const [missingStatus] = await messanger.send_request(
                "wrongsvc",
                "ping",
            );
            expectError(missingStatus, "NO_SUCH_SERVICE");

            const serviceName = "awesomesvc";
            const service = expectOk(
                await messanger.open_service(serviceName),
                "open service",
            );

            const [idleStatus] = await messanger.send_request(
                serviceName,
                "ping",
            );
            expectError(idleStatus, "CLOSED");

            const reverse = new ReverseService(service);
            const reverseTask = reverse.run();
            try {
                const requestBody = "djksjfkdsljf";
                const response = expectOk(
                    await messanger.send_request(serviceName, requestBody, 5_000),
                    "send reverse request",
                );
                expect(response).toBe(reverseBody(requestBody));
            } finally {
                await service.close();
                await reverseTask;
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "two player connections exchange cross-requests with reverse and duplicate services",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(messangerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(20);

            const playerA = await login("player", "awesome");
            const playerB = await login("player", "awesome");
            const messangerA = getMessanger(playerA).down_level();
            const messangerB = getMessanger(playerB).down_level();

            const reverseName = "reverse";
            const duplicateName = "duplicate";
            const reverseService = expectOk(
                await messangerA.open_service(reverseName),
                "open reverse service",
            );
            const duplicateService = expectOk(
                await messangerB.open_service(duplicateName),
                "open duplicate service",
            );

            const reverseTask = new ReverseService(reverseService).run();
            const duplicateTask = new DuplicateService(duplicateService).run();

            const aToReverse = "alpha";
            const aToDuplicate = "beta";
            const bToReverse = "gamma";
            const bToDuplicate = "delta";

            try {
                const [aReverse, aDuplicate, bReverse, bDuplicate] =
                    await Promise.all([
                        messangerA.send_request(reverseName, aToReverse, 5_000),
                        messangerA.send_request(duplicateName, aToDuplicate, 5_000),
                        messangerB.send_request(reverseName, bToReverse, 5_000),
                        messangerB.send_request(duplicateName, bToDuplicate, 5_000),
                    ]);

                expect(expectOk(aReverse, "A → reverse")).toBe(
                    reverseBody(aToReverse),
                );
                expect(expectOk(aDuplicate, "A → duplicate")).toBe(
                    duplicateBody(aToDuplicate),
                );
                expect(expectOk(bReverse, "B → reverse")).toBe(
                    reverseBody(bToReverse),
                );
                expect(expectOk(bDuplicate, "B → duplicate")).toBe(
                    duplicateBody(bToDuplicate),
                );
            } finally {
                await reverseService.close();
                await duplicateService.close();
                await Promise.all([reverseTask, duplicateTask]);
            }
        });
    },
);

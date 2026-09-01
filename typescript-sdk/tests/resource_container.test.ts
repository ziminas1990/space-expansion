import { expect, test } from "vitest";
import type {
    ResourceContainer,
    ResourceContainerContent,
} from "../highlevel/index.js";
import type { ResourceItem } from "../types/index.js";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    EngineState,
    General,
    makeMiner,
    Player,
    Position,
    ResourceContainerState,
    ResourceType,
    World,
} from "./configurator/index.js";
import {
    hasServerBinary,
    integrationTimeoutMs,
    withServer,
} from "./fixture.js";
import {
    Collector,
    expectOk,
    expectStatus,
    getCargo,
    getShip,
} from "./helpers/index.js";

function resourceContainerConfiguration(): Configuration {
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
                ships: [
                    makeMiner(
                        "miner-1",
                        new Position(0, 0),
                        new EngineState(),
                        new EngineState(),
                        new ResourceContainerState({
                            [ResourceType.Silicates]: 20_000,
                            [ResourceType.Metals]: 50_000,
                            [ResourceType.Ice]: 15_000,
                        }),
                    ),
                    makeMiner(
                        "miner-2",
                        new Position(10, 10),
                        new EngineState(),
                        new EngineState(),
                        new ResourceContainerState({
                            [ResourceType.Silicates]: 5_000,
                            [ResourceType.Metals]: 5_000,
                            [ResourceType.Ice]: 10_000,
                        }),
                    ),
                ],
            }),
        ],
    });
}

function resourceAmount(
    content: ResourceContainerContent,
    resourceType: string,
): number {
    return content.resources.find((item) => item.resource_type === resourceType)
        ?.amount ?? 0;
}

function expectAmount(
    content: ResourceContainerContent,
    resourceType: string,
    expected: number,
): void {
    expect(resourceAmount(content, resourceType)).toBeCloseTo(expected, 5);
}

function expectError(status: { is_ok(): boolean; what(): string }, code: string): void {
    expect(status.what()).toBe(code);
}

function sleep(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

async function monitorContent(
    cargo: ResourceContainer,
    journal: Collector<ResourceContainerContent>,
    stopped: { value: boolean },
): Promise<void> {
    expectStatus(
        await cargo.down_level().monitoring(async (content) => {
            if (content) {
                journal.push(content);
            }
            return !stopped.value;
        }),
        "cargo monitoring",
    );
}

test.skipIf(!hasServerBinary)(
    "returns cargo content",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(resourceContainerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(25);

            // 1. player logins
            const player = await login("player", "player");

            // 2. get miner cargo
            const cargo = getCargo(getShip(player, "miner-1"), "cargo");

            // 3. get cargo content and check amounts
            const content = expectOk(await cargo.get_content(), "get cargo content");
            expectAmount(content, ResourceType.Silicates, 20_000);
            expectAmount(content, ResourceType.Metals, 50_000);
            expectAmount(content, ResourceType.Ice, 15_000);
        });
    },
);

test.skipIf(!hasServerBinary)(
    "opens and closes a port",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(resourceContainerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(25);

            // 1. player logins
            const player = await login("player", "player");

            // 2. get miner cargo
            const cargo = getCargo(getShip(player, "miner-1"), "cargo");

            // 3. check port is not opened
            expect(cargo.opened_port).toBeUndefined();
            expectError(await cargo.close_port(), "PORT_IS_NOT_OPENED");

            // 4. open a port
            const accessKey = 12_456;
            const port = expectOk(await cargo.open_port(accessKey), "open port");
            expect(port).not.toBe(0);
            expect(cargo.opened_port?.[0]).toBe(port);
            expect(cargo.opened_port?.[1]).toBe(accessKey);

            // 5. opening again fails
            const [alreadyOpen, extraPort] = await cargo.open_port(accessKey * 2);
            expectError(alreadyOpen, "PORT_ALREADY_OPEN");
            expect(extraPort ?? 0).toBe(0);

            // 6. close the port
            expectStatus(await cargo.close_port(), "close port");
            expectError(await cargo.close_port(), "PORT_IS_NOT_OPENED");

            // 7. reopen the port
            const reopened = expectOk(await cargo.open_port(accessKey), "reopen port");
            expect(reopened).not.toBe(0);
        });
    },
);

test.skipIf(!hasServerBinary)(
    "transfers resources between containers",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(resourceContainerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(25);

            // 1. player logins
            const player = await login("player", "player");

            // 2. get both miner cargos
            const miner1Cargo = getCargo(getShip(player, "miner-1"), "cargo");
            const miner2Cargo = getCargo(getShip(player, "miner-2"), "cargo");

            // 3. open miner-2 port
            const accessKey = 12_456;
            const port = expectOk(
                await miner2Cargo.open_port(accessKey),
                "open miner-2 port",
            );
            expect(port).not.toBe(0);

            // 4. transfer metals from miner-1 to miner-2
            let transferred = 0;
            expectStatus(
                await miner1Cargo.transfer(
                    port,
                    accessKey,
                    { resource_type: "metals", amount: 30_000 },
                    (item: ResourceItem) => {
                        expect(item.resource_type).toBe("metals");
                        transferred += item.amount;
                    },
                ),
                "transfer metals",
            );
            expect(transferred).toBeCloseTo(30_000, 5);

            // 5. check miner-1 content after transfer
            const miner1Content = expectOk(
                await miner1Cargo.get_content(),
                "miner-1 content after transfer",
            );
            expectAmount(miner1Content, ResourceType.Metals, 20_000);
            expectAmount(miner1Content, ResourceType.Silicates, 20_000);
            expectAmount(miner1Content, ResourceType.Ice, 15_000);

            // 6. check miner-2 content after transfer
            const miner2Content = expectOk(
                await miner2Cargo.get_content(),
                "miner-2 content after transfer",
            );
            expectAmount(miner2Content, ResourceType.Metals, 35_000);
            expectAmount(miner2Content, ResourceType.Silicates, 5_000);
            expectAmount(miner2Content, ResourceType.Ice, 10_000);
        });
    },
);

test.skipIf(!hasServerBinary)(
    "rejects transfers to a closed or unauthorized port",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(resourceContainerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(25);

            // 1. player logins
            const player = await login("player", "player");

            // 2. get both miner cargos
            const miner1Cargo = getCargo(getShip(player, "miner-1"), "cargo");
            const miner2Cargo = getCargo(getShip(player, "miner-2"), "cargo");

            // 3. transfer to a closed port fails
            expectError(
                await miner1Cargo.transfer(
                    4,
                    123_456,
                    { resource_type: "metals", amount: 30_000 },
                ),
                "PORT_IS_NOT_OPENED",
            );

            // 4. open miner-2 port
            const accessKey = 12_456;
            const port = expectOk(
                await miner2Cargo.open_port(accessKey),
                "open miner-2 port",
            );
            expect(port).not.toBe(0);

            // 5. transfer with a wrong access key fails
            expectError(
                await miner1Cargo.transfer(
                    port,
                    accessKey - 1,
                    { resource_type: "metals", amount: 30_000 },
                ),
                "INVALID_ACCESS_KEY",
            );
        });
    },
);

test.skipIf(!hasServerBinary)(
    "monitors transfer progress in both containers",
    { timeout: integrationTimeoutMs },
    async () => {
        await withServer(resourceContainerConfiguration(), async ({ login, clock }) => {
            await clock.fastForward(10);

            // 1. player logins
            const player = await login("player", "player");

            // 2. get both miner cargos
            const miner1Cargo = getCargo(getShip(player, "miner-1"), "cargo");
            const miner2Cargo = getCargo(getShip(player, "miner-2"), "cargo");

            // 3. open miner-2 port
            const accessKey = 12_456;
            const port = expectOk(
                await miner2Cargo.open_port(accessKey),
                "open miner-2 port",
            );
            expect(port).not.toBe(0);

            // 4. start monitoring both cargos
            const transactions = new Collector<ResourceItem>();
            const miner1Journal = new Collector<ResourceContainerContent>();
            const miner2Journal = new Collector<ResourceContainerContent>();
            const stopMiner1 = { value: false };
            const stopMiner2 = { value: false };

            const miner1Monitoring = monitorContent(
                miner1Cargo,
                miner1Journal,
                stopMiner1,
            );
            const miner2Monitoring = monitorContent(
                miner2Cargo,
                miner2Journal,
                stopMiner2,
            );

            try {
                // 5. wait for initial content
                await miner1Journal.waitForCount(1, "miner-1 initial content");
                await miner2Journal.waitForCount(1, "miner-2 initial content");

                // 6. transfer metals
                expectStatus(
                    await miner1Cargo.transfer(
                        port,
                        accessKey,
                        { resource_type: "metals", amount: 30_000 },
                        transactions.callback,
                    ),
                    "transfer metals",
                );

                // 7. wait for monitoring updates
                await sleep(200);
            } finally {
                stopMiner1.value = true;
                stopMiner2.value = true;
                await Promise.all([miner1Monitoring, miner2Monitoring]);
            }

            // 8. check journals match transfer events
            expect(miner1Journal.length).toBe(miner2Journal.length);
            expect(miner1Journal.length).toBe(transactions.length + 1);
            expect(miner2Journal.length).toBe(transactions.length + 1);

            // 9. check miner-1 content decreases by each transaction
            let miner1Content = miner1Journal.items[0]!;
            for (let i = 0; i < transactions.length; i += 1) {
                const transaction = transactions.items[i]!;
                const updated = miner1Journal.items[i + 1]!;
                expectAmount(
                    updated,
                    transaction.resource_type,
                    resourceAmount(miner1Content, transaction.resource_type)
                        - transaction.amount,
                );
                miner1Content = updated;
            }

            // 10. check miner-2 content increases by each transaction
            let miner2Content = miner2Journal.items[0]!;
            for (let i = 0; i < transactions.length; i += 1) {
                const transaction = transactions.items[i]!;
                const updated = miner2Journal.items[i + 1]!;
                expectAmount(
                    updated,
                    transaction.resource_type,
                    resourceAmount(miner2Content, transaction.resource_type)
                        + transaction.amount,
                );
                miner2Content = updated;
            }
        });
    },
);

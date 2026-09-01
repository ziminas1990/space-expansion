import { expect, test } from "vitest";
import type {
    ResourceContainer,
    ResourceContainerContent,
    ShipyardStatus,
} from "../highlevel/index.js";
import type { ResourceItem } from "../types/index.js";
import {
    ApplicationMode,
    Configuration,
    DefaultBlueprints,
    EngineState,
    General,
    makeStation,
    Player,
    Position,
    ResourceContainerState,
    ResourceType,
    ResourcesList,
    ShipBlueprint,
    World,
    type BlueprintsDB,
    type ResourceAmounts,
} from "./configurator/index.js";
import {
    hasServerBinary,
    integrationTimeoutMs,
    withServer,
} from "./fixture.js";
import type { IngameClock } from "./ingame_clock.js";
import {
    expectOk,
    expectStatus,
    getCargo,
    getShip,
    getShipyard,
} from "./helpers/index.js";

const progressDelta = 0.001;

function shipyardConfiguration(): Configuration {
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
                ships: [
                    makeStation(
                        "SweetHome",
                        new Position(0, 0),
                        new EngineState(),
                        new ResourceContainerState({
                            [ResourceType.Metals]: 200_000,
                            [ResourceType.Silicates]: 40_000,
                        }),
                    ),
                ],
            }),
        ],
    });
}

function requireShipBlueprint(blueprints: BlueprintsDB, name: string): ShipBlueprint {
    const blueprint = blueprints.blueprints.get(`Ship/${name}`);
    if (!(blueprint instanceof ShipBlueprint)) {
        throw new Error(`Ship blueprint '${name}' not found`);
    }
    return blueprint;
}

function shipyardLaborPerSec(blueprints: BlueprintsDB, name: string): number {
    const blueprint = blueprints.blueprints.get(`Shipyard/${name}`);
    if (blueprint === undefined) {
        throw new Error(`Shipyard blueprint '${name}' not found`);
    }
    const productivity = blueprint.toPod().productivity;
    if (typeof productivity !== "number") {
        throw new Error(`Shipyard blueprint '${name}' has no productivity`);
    }
    return productivity;
}

function resourcesFromContent(content: ResourceContainerContent): ResourcesList {
    const amounts: ResourceAmounts = {};
    for (const item of content.resources) {
        if (item.resource_type === "unknown") {
            continue;
        }
        amounts[item.resource_type as ResourceType] = item.amount;
    }
    return new ResourcesList(amounts);
}

function physicalExpenses(expenses: ResourcesList): Array<[string, number]> {
    return Object.entries(expenses.resources).filter(
        (entry): entry is [string, number] =>
            entry[0] !== ResourceType.Labor
            && entry[1] !== undefined
            && entry[1] > 0,
    );
}

async function transferExpenses(
    warehouse: ResourceContainer,
    port: number,
    accessKey: number,
    expenses: ResourcesList,
    multiplier: number,
    description: string,
): Promise<void> {
    for (const [resource, amount] of physicalExpenses(expenses)) {
        expectStatus(
            await warehouse.transfer(port, accessKey, {
                resource_type: resource as ResourceItem["resource_type"],
                amount: amount * multiplier,
            }),
            `${description}: ${resource}`,
        );
    }
}

class ProgressTracker {
    private status: ShipyardStatus | undefined;
    private progress = 0;
    private reported = false;

    constructor(private readonly proceedClock: () => Promise<void>) {}

    readonly onProgress = (status: ShipyardStatus, progress: number): void => {
        this.status = status;
        this.progress = progress;
        this.reported = true;
    };

    async nextReport(
        timeoutMs = 3_000,
    ): Promise<[ShipyardStatus | undefined, number | undefined]> {
        const stopAt = Date.now() + timeoutMs;
        while (!this.reported) {
            if (Date.now() >= stopAt) {
                return [undefined, undefined];
            }
            await this.proceedClock();
        }
        this.reported = false;
        return [this.status, this.progress];
    }

    async waitReport(
        expected: ShipyardStatus,
        timeoutMs = 3_000,
    ): Promise<number | undefined> {
        const stopAt = Date.now() + timeoutMs;
        while (Date.now() < stopAt) {
            const [status, progress] = await this.nextReport(stopAt - Date.now());
            if (status === expected) {
                return progress;
            }
            if (status === undefined) {
                return undefined;
            }
        }
        return undefined;
    }

    waitCompleteReport(timeoutMs = 3_000): Promise<number | undefined> {
        return this.waitReport("BUILD_COMPLETE", timeoutMs);
    }

    waitFrozenReport(timeoutMs = 3_000): Promise<number | undefined> {
        return this.waitReport("BUILD_FROZEN", timeoutMs);
    }

    waitProgressReport(timeoutMs = 3_000): Promise<number | undefined> {
        return this.waitReport("BUILD_IN_PROGRESS", timeoutMs);
    }
}

function proceedBuild(clock: IngameClock): () => Promise<void> {
    return async () => {
        await clock.proceed(450, 1_000, 50_000);
    };
}

test.skipIf(!hasServerBinary)(
    "returns the shipyard specification",
    { timeout: integrationTimeoutMs },
    async () => {
        const configuration = shipyardConfiguration();
        await withServer(configuration, async ({ login, clock }) => {
            await clock.fastForward(20);

            // 1. player logins
            const player = await login("player", "awesome");

            // 2. get station shipyards
            const station = getShip(player, "SweetHome");
            const shipyardMedium = getShipyard(station, "shipyard-medium");
            const shipyardLarge = getShipyard(station, "shipyard-large");

            // 3. check medium shipyard specification
            const mediumSpec = expectOk(
                await shipyardMedium.get_specification(),
                "medium shipyard specification",
            );
            expect(mediumSpec.labor_per_sec).toBe(
                shipyardLaborPerSec(configuration.blueprints, "Medium Shipyard"),
            );

            // 4. check large shipyard specification
            const largeSpec = expectOk(
                await shipyardLarge.get_specification(),
                "large shipyard specification",
            );
            expect(largeSpec.labor_per_sec).toBe(
                shipyardLaborPerSec(configuration.blueprints, "Large Shipyard"),
            );
        });
    },
);

test.skipIf(!hasServerBinary)(
    "builds a ship through frozen, progress, and complete",
    { timeout: integrationTimeoutMs },
    async () => {
        const configuration = shipyardConfiguration();
        await withServer(configuration, async ({ login, clock }) => {
            await clock.fastForward(20);

            const minerBlueprint = requireShipBlueprint(
                configuration.blueprints,
                "Miner",
            );
            const shipExpenses = configuration.blueprints.shipExpenses(minerBlueprint);

            // 1. player logins
            const player = await login("player", "awesome");

            // 2. get station, warehouse, shipyard container and shipyard
            const station = getShip(player, "SweetHome");
            const warehouse = getCargo(station, "warehouse");
            const shipyardContainer = getCargo(station, "shipyard-container");
            const shipyard = getShipyard(station, "shipyard-large");

            // 3. bind shipyard to cargo
            expectStatus(
                await shipyard.bind_to_cargo("shipyard-container"),
                "bind to shipyard-container",
            );

            // 4. open shipyard container port
            const accessKey = 1_234;
            const port = expectOk(
                await shipyardContainer.open_port(accessKey),
                "open shipyard container port",
            );

            // 5. check warehouse has enough resources
            const content = expectOk(await warehouse.get_content(0), "warehouse content");
            expect(resourcesFromContent(content).contains(shipExpenses)).toBe(true);

            // 6. start building a miner
            const tracker = new ProgressTracker(proceedBuild(clock));
            const buildTask = shipyard.build_ship(
                minerBlueprint.id.toPod(),
                "SCV",
                tracker.onProgress,
            );
            try {
                // 7. wait for frozen report (empty container)
                let progress = await tracker.waitFrozenReport();
                expect(progress).toBeDefined();
                expect(progress).toBeCloseTo(0);

                // 8. transfer resources in 10% steps
                for (let i = 1; i <= 10; i += 1) {
                    // 8.1 fast-forward and transfer 10% of required resources
                    await clock.fastForward(50, 10_000);
                    await transferExpenses(
                        warehouse,
                        port,
                        accessKey,
                        shipExpenses,
                        0.1,
                        `iteration ${i}`,
                    );

                    // 8.2 play and wait for progress to reach 10% * i
                    await clock.play();
                    while ((progress ?? 0) + progressDelta < 0.1 * i) {
                        progress = await tracker.waitProgressReport();
                        expect(progress, `progress report ${i}`).toBeDefined();
                    }
                    expect(Math.abs((progress ?? 0) - 0.1 * i)).toBeLessThanOrEqual(
                        progressDelta,
                    );

                    // 8.3 wait for frozen reports while the container is empty
                    if (i < 10) {
                        for (let j = 0; j < 10; j += 1) {
                            progress = await tracker.waitFrozenReport();
                            expect(progress, `frozen report ${i}.${j}`).toBeDefined();
                            expect(
                                Math.abs((progress ?? 0) - 0.1 * i),
                            ).toBeLessThanOrEqual(progressDelta);
                        }
                    }
                }

                // 9. wait for build complete
                progress = await tracker.waitCompleteReport();
                expect(progress).toBeCloseTo(1);

                // 10. check build succeeded
                expectOk(await buildTask, "build miner");
            } finally {
                if (shipyard.is_building()) {
                    await shipyard.cancel_build();
                }
                await buildTask.catch(() => undefined);
            }
        });
    },
);

test.skipIf(!hasServerBinary)(
    "builds multiple ships sequentially",
    { timeout: integrationTimeoutMs },
    async () => {
        const configuration = shipyardConfiguration();
        await withServer(configuration, async ({ login, clock }) => {
            await clock.fastForward(20);

            const totalProbes = 2;
            const probeBlueprint = requireShipBlueprint(
                configuration.blueprints,
                "Probe",
            );
            const shipExpenses = configuration.blueprints.shipExpenses(probeBlueprint);

            // 1. player logins
            const player = await login("player", "awesome");

            // 2. get station, warehouse, shipyard container and shipyard
            const station = getShip(player, "SweetHome");
            const warehouse = getCargo(station, "warehouse");
            const shipyardContainer = getCargo(station, "shipyard-container");
            const shipyard = getShipyard(station, "shipyard-large");

            // 3. bind shipyard to cargo
            expectStatus(
                await shipyard.bind_to_cargo("shipyard-container"),
                "bind to shipyard-container",
            );

            // 4. open shipyard container port
            const accessKey = 1_234;
            const port = expectOk(
                await shipyardContainer.open_port(accessKey),
                "open shipyard container port",
            );

            // 5. transfer resources for probes
            await transferExpenses(
                warehouse,
                port,
                accessKey,
                shipExpenses,
                totalProbes + 1,
                "load shipyard container",
            );

            // 6. check warehouse still has resources for 10 probes
            const content = expectOk(await warehouse.get_content(0), "warehouse content");
            expect(
                resourcesFromContent(content).contains(shipExpenses.multiply(10)),
            ).toBe(true);

            // 7. build probes sequentially
            for (let i = 0; i < totalProbes; i += 1) {
                expectOk(
                    await shipyard.build_ship(
                        probeBlueprint.id.toPod(),
                        `probe_${i}`,
                    ),
                    `build probe_${i}`,
                );
            }
        });
    },
);

import {
    type ResourceAmounts,
    ResourcesList,
    ResourceType,
} from "./resources.js";

export enum ModuleType {
    Ship = "Ship",
    Engine = "Engine",
    PassiveScanner = "PassiveScanner",
    ResourceContainer = "ResourceContainer",
    AsteroidMiner = "AsteroidMiner",
    Shipyard = "Shipyard",
}

export class BlueprintId {
    constructor(
        readonly type: ModuleType,
        readonly name: string,
    ) {}

    static engine(name: string): BlueprintId {
        return new BlueprintId(ModuleType.Engine, name);
    }

    verify(): void {
        if (this.name.length === 0) {
            throw new Error("Blueprint name cannot be empty");
        }
    }

    toPod(): string {
        this.verify();
        return `${this.type}/${this.name}`;
    }
}

export class Blueprint {
    constructor(
        readonly id: BlueprintId,
        readonly expenses: ResourcesList,
        private readonly properties: Readonly<Record<string, unknown>> = {},
    ) {}

    verify(): void {
        this.id.verify();
        this.expenses.verify();
    }

    toPod(): Record<string, unknown> {
        this.verify();
        return {
            expenses: this.expenses.toPod(),
            ...this.properties,
        };
    }
}

export class ShipBlueprint extends Blueprint {
    constructor(
        name: string,
        readonly radius: number,
        readonly weight: number,
        readonly modules: Readonly<Record<string, BlueprintId>>,
        expenses: ResourcesList,
    ) {
        super(new BlueprintId(ModuleType.Ship, name), expenses);
    }

    override verify(): void {
        super.verify();
        if (this.radius <= 0.01) {
            throw new Error(
                `Ship blueprint '${this.id.name}' has invalid radius`,
            );
        }
        if (this.weight <= 0.01) {
            throw new Error(
                `Ship blueprint '${this.id.name}' has invalid weight`,
            );
        }
    }

    override toPod(): Record<string, unknown> {
        this.verify();
        return {
            ...super.toPod(),
            radius: this.radius,
            weight: this.weight,
            modules: Object.fromEntries(
                Object.entries(this.modules).map((
                    [name, id],
                ) => [name, id.toPod()]),
            ),
        };
    }
}

export class BlueprintsDB {
    readonly blueprints = new Map<string, Blueprint>();

    constructor(blueprints: readonly Blueprint[] = []) {
        for (const blueprint of blueprints) {
            this.addBlueprint(blueprint);
        }
    }

    addBlueprint(blueprint: Blueprint): this {
        this.blueprints.set(blueprint.id.toPod(), blueprint);
        return this;
    }

    has(id: BlueprintId): boolean {
        return this.blueprints.has(id.toPod());
    }

    verify(): void {
        for (const [key, blueprint] of this.blueprints) {
            if (key !== blueprint.id.toPod()) {
                throw new Error(`Blueprint key '${key}' does not match its id`);
            }
            blueprint.verify();

            if (blueprint instanceof ShipBlueprint) {
                for (const moduleId of Object.values(blueprint.modules)) {
                    if (moduleId.type === ModuleType.Ship) {
                        throw new Error(
                            "A ship module cannot reference a ship blueprint",
                        );
                    }
                    if (!this.has(moduleId)) {
                        throw new Error(
                            `Blueprint '${moduleId.toPod()}' does not exist`,
                        );
                    }
                }
            }
        }
    }

    shipExpenses(ship: ShipBlueprint): ResourcesList {
        const total = new ResourcesList(ship.expenses.resources);
        for (const moduleId of Object.values(ship.modules)) {
            const moduleBlueprint = this.blueprints.get(moduleId.toPod());
            if (moduleBlueprint === undefined) {
                throw new Error(
                    `Blueprint '${moduleId.toPod()}' does not exist`,
                );
            }
            total.add(moduleBlueprint.expenses);
        }
        return total;
    }

    toPod(): Record<string, unknown> {
        this.verify();

        const modules: Record<string, Record<string, unknown>> = {};
        const ships: Record<string, unknown> = {};
        for (const blueprint of this.blueprints.values()) {
            if (blueprint.id.type === ModuleType.Ship) {
                ships[blueprint.id.name] = blueprint.toPod();
                continue;
            }

            const group = (modules[blueprint.id.type] ??= {});
            group[blueprint.id.name] = blueprint.toPod();
        }
        return { Modules: modules, Ships: ships };
    }
}

export class DefaultBlueprints extends BlueprintsDB {
    constructor() {
        super(createDefaultBlueprints());
    }
}

function createDefaultBlueprints(): Blueprint[] {
    const blueprints: Blueprint[] = [
        moduleBlueprint(
            ModuleType.Engine,
            "Tiny Chemical engine",
            { max_thrust: 1_000 },
            { metals: 200, silicates: 100, labor: 20 },
        ),
        moduleBlueprint(
            ModuleType.Engine,
            "Small Chemical engine",
            { max_thrust: 10_000 },
            { metals: 600, silicates: 200, labor: 50 },
        ),
        moduleBlueprint(
            ModuleType.Engine,
            "Tiny Ion engine",
            { max_thrust: 100 },
            { metals: 100, silicates: 20, labor: 20 },
        ),
        moduleBlueprint(
            ModuleType.Engine,
            "Small Ion engine",
            { max_thrust: 500 },
            { metals: 300, silicates: 50, labor: 60 },
        ),
        moduleBlueprint(
            ModuleType.Engine,
            "Regular Ion engine",
            { max_thrust: 2_000 },
            { metals: 1_000, silicates: 120, labor: 180 },
        ),
        moduleBlueprint(
            ModuleType.Engine,
            "Tiny Nuclear engine",
            { max_thrust: 3_000 },
            { metals: 1_500, silicates: 400, labor: 200 },
        ),
        moduleBlueprint(
            ModuleType.Engine,
            "Small Nuclear engine",
            { max_thrust: 30_000 },
            { metals: 10_000, silicates: 2_800, labor: 1_600 },
        ),
        moduleBlueprint(
            ModuleType.Engine,
            "Regular Nuclear engine",
            { max_thrust: 400_000 },
            { metals: 35_000, silicates: 15_000, labor: 9_500 },
        ),
        moduleBlueprint(
            ModuleType.PassiveScanner,
            "Basic Scanner",
            { max_scanning_radius_km: 50, edge_update_time_ms: 2_000 },
            { metals: 200, silicates: 200, labor: 20 },
        ),
        moduleBlueprint(
            ModuleType.PassiveScanner,
            "Military Scanner",
            { max_scanning_radius_km: 150, edge_update_time_ms: 40_000 },
            { metals: 800, silicates: 800, labor: 80 },
        ),
        moduleBlueprint(
            ModuleType.PassiveScanner,
            "Station Scanner",
            { max_scanning_radius_km: 250, edge_update_time_ms: 6_000 },
            { metals: 2_500, silicates: 2_500, labor: 400 },
        ),
        moduleBlueprint(
            ModuleType.ResourceContainer,
            "Tiny Resource Container",
            { volume: 20 },
            { metals: 400, silicates: 6, labor: 20 },
        ),
        moduleBlueprint(
            ModuleType.ResourceContainer,
            "Small Resource Container",
            { volume: 125 },
            { metals: 1_200, silicates: 25, labor: 80 },
        ),
        moduleBlueprint(
            ModuleType.ResourceContainer,
            "Medium Resource Container",
            { volume: 500 },
            { metals: 3_000, silicates: 60, labor: 250 },
        ),
        moduleBlueprint(
            ModuleType.ResourceContainer,
            "Station Resource Container",
            { volume: 1_500 },
            { metals: 5_000, silicates: 140, labor: 600 },
        ),
        moduleBlueprint(
            ModuleType.AsteroidMiner,
            "Toy Miner",
            {
                max_distance: 500,
                cycle_time_ms: 10_000,
                yield_per_cycle: 250,
            },
            { metals: 2_000, silicates: 100, labor: 50 },
        ),
        moduleBlueprint(
            ModuleType.Shipyard,
            "Small Shipyard",
            { productivity: 5 },
            { metals: 20_000, silicates: 10_000, labor: 25_000 },
        ),
        moduleBlueprint(
            ModuleType.Shipyard,
            "Medium Shipyard",
            { productivity: 10 },
            { metals: 33_000, silicates: 15_000, labor: 45_000 },
        ),
        moduleBlueprint(
            ModuleType.Shipyard,
            "Large Shipyard",
            { productivity: 25 },
            { metals: 60_000, silicates: 30_000, labor: 80_000 },
        ),
    ];

    blueprints.push(
        new ShipBlueprint(
            "Probe",
            2,
            220,
            {
                main_engine: id(ModuleType.Engine, "Tiny Chemical engine"),
                additional_engine: id(ModuleType.Engine, "Tiny Ion engine"),
            },
            expenses({ metals: 200, silicates: 20, labor: 100 }),
        ),
        new ShipBlueprint(
            "Miner",
            80,
            80_000,
            {
                main_engine: id(ModuleType.Engine, "Regular Nuclear engine"),
                additional_engine: id(ModuleType.Engine, "Regular Ion engine"),
                perceiver: id(ModuleType.PassiveScanner, "Basic Scanner"),
                cargo: id(
                    ModuleType.ResourceContainer,
                    "Small Resource Container",
                ),
                tiny_cargo: id(
                    ModuleType.ResourceContainer,
                    "Tiny Resource Container",
                ),
                miner: id(ModuleType.AsteroidMiner, "Toy Miner"),
            },
            expenses({ metals: 70_000, silicates: 10_000, labor: 20_000 }),
        ),
        new ShipBlueprint(
            "Station",
            800,
            20_000_000,
            {
                engine: id(ModuleType.Engine, "Regular Nuclear engine"),
                perceiver: id(ModuleType.PassiveScanner, "Station Scanner"),
                warehouse: id(
                    ModuleType.ResourceContainer,
                    "Station Resource Container",
                ),
                "shipyard-container": id(
                    ModuleType.ResourceContainer,
                    "Station Resource Container",
                ),
                "shipyard-medium": id(ModuleType.Shipyard, "Medium Shipyard"),
                "shipyard-large": id(ModuleType.Shipyard, "Large Shipyard"),
            },
            expenses({
                metals: 17_000_000,
                silicates: 3_000_000,
                labor: 3_000_000,
            }),
        ),
    );
    return blueprints;
}

function moduleBlueprint(
    type: Exclude<ModuleType, ModuleType.Ship>,
    name: string,
    properties: Readonly<Record<string, unknown>>,
    resourceAmounts: WireResourceAmounts,
): Blueprint {
    return new Blueprint(
        id(type, name),
        expenses(resourceAmounts),
        properties,
    );
}

function id(type: ModuleType, name: string): BlueprintId {
    return new BlueprintId(type, name);
}

type WireResourceAmounts = Partial<Record<ResourceType, number>>;

function expenses(resourceAmounts: WireResourceAmounts): ResourcesList {
    return new ResourcesList(resourceAmounts as ResourceAmounts);
}

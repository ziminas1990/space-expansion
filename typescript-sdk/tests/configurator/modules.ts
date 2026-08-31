import { PhysicalResources, type ResourceAmounts } from "./resources.js";
import { Position, Vector } from "./world.js";

export interface ModuleState {
    verify(): void;
    toPod(): unknown;
}

export class BaseModuleState implements ModuleState {
    verify(): void {}

    toPod(): Record<string, unknown> {
        this.verify();
        return {};
    }
}

export class EngineState extends BaseModuleState {
    constructor(public thrust = new Vector(0, 0)) {
        super();
    }

    setThrust(thrust: Vector): this {
        this.thrust = thrust;
        return this;
    }

    override verify(): void {
        this.thrust.verify();
    }

    override toPod(): { x: number; y: number } {
        this.verify();
        return this.thrust.toPod();
    }
}

export class ResourceContainerState extends BaseModuleState {
    readonly content: PhysicalResources;

    constructor(content: ResourceAmounts | PhysicalResources = {}) {
        super();
        this.content = content instanceof PhysicalResources
            ? content
            : new PhysicalResources(content);
    }

    override verify(): void {
        this.content.verify();
    }

    override toPod(): Record<string, number> {
        this.verify();
        return this.content.toPod();
    }
}

export enum ShipType {
    Probe = "Probe",
    Miner = "Miner",
    Station = "Station",
}

export interface ShipOptions {
    name: string;
    shipType: ShipType | string;
    position: Position;
    modules?: Readonly<Record<string, ModuleState>>;
}

export class Ship {
    readonly shipName: string;
    readonly shipType: string;
    position: Position;
    readonly modules = new Map<string, ModuleState>();

    constructor(options: ShipOptions) {
        this.shipName = options.name;
        this.shipType = options.shipType;
        this.position = options.position;
        for (const [name, state] of Object.entries(options.modules ?? {})) {
            this.modules.set(name, state);
        }
    }

    setPosition(position: Position): this {
        this.position = position;
        return this;
    }

    configureModule(name: string, state: ModuleState): this {
        if (this.modules.has(name)) {
            throw new Error(`Module '${name}' is already configured`);
        }
        this.modules.set(name, state);
        return this;
    }

    verify(): void {
        if (this.shipName.length === 0) {
            throw new Error("Ship name cannot be empty");
        }
        if (this.shipType.length === 0) {
            throw new Error("Ship type cannot be empty");
        }
        this.position.verify();
        for (const [name, state] of this.modules) {
            if (name.length === 0) {
                throw new Error("Module slot name cannot be empty");
            }
            state.verify();
        }
    }

    toPod(): Record<string, unknown> {
        this.verify();
        return {
            ...this.position.toPod(),
            modules: Object.fromEntries(
                [...this.modules].map(([name, state]) => [name, state.toPod()]),
            ),
        };
    }
}

export function makeProbe(
    name: string,
    position: Position,
    mainEngine = new EngineState(),
    additionalEngine = new EngineState(),
): Ship {
    return new Ship({
        name,
        shipType: ShipType.Probe,
        position,
        modules: {
            main_engine: mainEngine,
            additional_engine: additionalEngine,
        },
    });
}

export function makeMiner(
    name: string,
    position: Position,
    mainEngine = new EngineState(),
    additionalEngine = new EngineState(),
    cargo = new ResourceContainerState(),
    tinyCargo = new ResourceContainerState(),
): Ship {
    return new Ship({
        name,
        shipType: ShipType.Miner,
        position,
        modules: {
            main_engine: mainEngine,
            additional_engine: additionalEngine,
            cargo,
            tiny_cargo: tinyCargo,
        },
    });
}

export function makeStation(
    name: string,
    position: Position,
    engine = new EngineState(),
    warehouse = new ResourceContainerState(),
    shipyardContainer = new ResourceContainerState(),
): Ship {
    return new Ship({
        name,
        shipType: ShipType.Station,
        position,
        modules: {
            engine,
            warehouse,
            "shipyard-container": shipyardContainer,
        },
    });
}

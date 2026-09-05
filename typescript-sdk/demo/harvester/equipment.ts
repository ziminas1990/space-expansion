import {
    ModuleType,
    type AsteroidMiner,
    type Engine,
    type PassiveScanner,
    type ResourceContainer,
    type Ship,
    type Shipyard,
} from "../../highlevel/index.js";

export function has_modules(ship: Ship, types: ModuleType[]): boolean {
    return types.every((type) => ship.get_all(type).length > 0);
}

export async function find_most_powerful_engine(
    ship: Ship,
): Promise<Engine | undefined> {
    let best: Engine | undefined;
    let best_thrust = -Infinity;
    for (const engine of ship.get_all(ModuleType.ENGINE)) {
        const [status, spec] = await engine.get_specification();
        if (!status.is_ok() || spec === undefined) {
            continue;
        }
        if (spec.max_thrust > best_thrust) {
            best = engine;
            best_thrust = spec.max_thrust;
        }
    }
    return best;
}

export async function find_most_productive_shipyard(
    ship: Ship,
): Promise<Shipyard | undefined> {
    let best: Shipyard | undefined;
    let best_labor = -Infinity;
    for (const shipyard of ship.get_all(ModuleType.SHIPYARD)) {
        const [status, spec] = await shipyard.get_specification();
        if (!status.is_ok() || spec === undefined) {
            continue;
        }
        if (spec.labor_per_sec > best_labor) {
            best = shipyard;
            best_labor = spec.labor_per_sec;
        }
    }
    return best;
}

export async function find_most_voluminous_container(
    ship: Ship,
): Promise<ResourceContainer | undefined> {
    let best: ResourceContainer | undefined;
    let best_volume = -Infinity;
    for (const container of ship.get_all(ModuleType.RESOURCE_CONTAINER)) {
        const [status, content] = await container.get_content();
        if (!status.is_ok() || content === undefined) {
            continue;
        }
        if (content.volume > best_volume) {
            best = container;
            best_volume = content.volume;
        }
    }
    return best;
}

export async function find_most_free_container(
    ship: Ship,
): Promise<ResourceContainer | undefined> {
    let best: ResourceContainer | undefined;
    let best_free = -Infinity;
    for (const container of ship.get_all(ModuleType.RESOURCE_CONTAINER)) {
        const [status, content] = await container.get_content();
        if (!status.is_ok() || content === undefined) {
            continue;
        }
        const free = content.volume - content.used;
        if (free > best_free) {
            best = container;
            best_free = free;
        }
    }
    return best;
}

export async function find_most_efficient_miner(
    ship: Ship,
): Promise<AsteroidMiner | undefined> {
    let best: AsteroidMiner | undefined;
    let best_rate = -Infinity;
    for (const miner of ship.get_all(ModuleType.ASTEROID_MINER)) {
        const [status, spec] = await miner.get_specification();
        if (!status.is_ok() || spec === undefined || spec.cycle_time_ms <= 0) {
            continue;
        }
        const rate = spec.yield_per_cycle / spec.cycle_time_ms;
        if (rate > best_rate) {
            best = miner;
            best_rate = rate;
        }
    }
    return best;
}

export async function find_most_ranged_scanner(
    ship: Ship,
): Promise<PassiveScanner | undefined> {
    let best: PassiveScanner | undefined;
    let best_radius = -Infinity;
    for (const scanner of ship.get_all(ModuleType.PASSIVE_SCANNER)) {
        const [status, spec] = await scanner.get_specification();
        if (!status.is_ok() || spec === undefined) {
            continue;
        }
        if (spec.scanning_radius_km > best_radius) {
            best = scanner;
            best_radius = spec.scanning_radius_km;
        }
    }
    return best;
}

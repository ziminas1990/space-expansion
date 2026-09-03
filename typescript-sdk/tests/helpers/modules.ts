import {
    ModuleType,
    type AsteroidMiner,
    type AsteroidScanner,
    type BlueprintsLibrary,
    type Engine,
    type HighlevelModule,
    type Messanger,
    type PassiveScanner,
    type Player,
    type ResourceContainer,
    type Ship,
    type Shipyard,
    type SystemClock,
} from "../../highlevel/index.js";
import { waitFor } from "./wait.js";

export function getShip(player: Player, name: string): Ship {
    const ship = player.ships.find((item) => item.name === name);
    if (ship === undefined) {
        throw new Error(`Ship '${name}' not found`);
    }
    return ship;
}

export function getAllShips(player: Player): Ship[] {
    return player.ships;
}

export async function waitForShip(
    player: Player,
    name: string,
    timeoutMs = 2_000,
): Promise<Ship> {
    await waitFor(
        () => player.ships.some((item) => item.name === name),
        `ship '${name}'`,
        timeoutMs,
    );
    return getShip(player, name);
}

export function getModule<T extends ModuleType>(
    ship: Ship,
    type: T,
    name: string,
): Extract<HighlevelModule, { type: T }> {
    const module = ship.get_by_name(type, name);
    if (module === undefined) {
        throw new Error(`${type} '${name}' not found on ship '${ship.name}'`);
    }
    return module;
}

export function getCargo(ship: Ship, name: string): ResourceContainer {
    return getModule(ship, ModuleType.RESOURCE_CONTAINER, name);
}

export function getEngine(ship: Ship, name: string): Engine {
    return getModule(ship, ModuleType.ENGINE, name);
}

export function getAllEngines(ship: Ship): Engine[] {
    return ship.get_all(ModuleType.ENGINE);
}

export async function getMostPowerfulEngine(ship: Ship): Promise<Engine> {
    let best: Engine | undefined;
    let bestThrust = -Infinity;
    for (const engine of getAllEngines(ship)) {
        const [status, spec] = await engine.get_specification();
        if (!status.is_ok() || spec === undefined) {
            continue;
        }
        if (spec.max_thrust > bestThrust) {
            best = engine;
            bestThrust = spec.max_thrust;
        }
    }
    if (best === undefined) {
        throw new Error(`No engines on ship '${ship.name}'`);
    }
    return best;
}

export function getAsteroidMiner(ship: Ship, name: string): AsteroidMiner {
    return getModule(ship, ModuleType.ASTEROID_MINER, name);
}

export function getPassiveScanner(ship: Ship, name: string): PassiveScanner {
    return getModule(ship, ModuleType.PASSIVE_SCANNER, name);
}

export function getShipyard(ship: Ship, name: string): Shipyard {
    return getModule(ship, ModuleType.SHIPYARD, name);
}

export function getAsteroidScanner(ship: Ship, name: string): AsteroidScanner {
    return getModule(ship, ModuleType.ASTEROID_SCANNER, name);
}

export function getBlueprintsLibrary(player: Player): BlueprintsLibrary {
    const library = player.blueprints_library();
    if (library === undefined) {
        throw new Error("BlueprintsLibrary not found");
    }
    return library;
}

export function getSystemClock(player: Player): SystemClock {
    const clock = player.system_clock();
    if (clock === undefined) {
        throw new Error("SystemClock not found");
    }
    return clock;
}

export function getMessanger(player: Player): Messanger {
    const messanger = player.messanger();
    if (messanger === undefined) {
        throw new Error("Messanger not found");
    }
    return messanger;
}

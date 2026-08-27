import { AsteroidMiner } from "./asteroid_miner.js";
import { AsteroidScanner } from "./asteroid_scanner.js";
import { BlueprintsLibrary } from "./blueprints_library.js";
import { CelestialScanner } from "./celestial_scanner.js";
import { Engine } from "./engine.js";
import { Messanger } from "./messanger.js";
import { PassiveScanner } from "./passive_scanner.js";
import { ResourceContainer } from "./resource_container.js";
import { Ship } from "./ship.js";
import { Shipyard } from "./shipyard.js";
import { SystemClock } from "./system_clock.js";

// Server reports a ship as "Ship/<subtype>", every other module type is
// reported as is.
export const ModuleType = {
    SHIP: "Ship/",
    SYSTEM_CLOCK: "SystemClock",
    ENGINE: "Engine",
    RESOURCE_CONTAINER: "ResourceContainer",
    CELESTIAL_SCANNER: "CelestialScanner",
    PASSIVE_SCANNER: "PassiveScanner",
    ASTEROID_SCANNER: "AsteroidScanner",
    ASTEROID_MINER: "AsteroidMiner",
    SHIPYARD: "Shipyard",
    BLUEPRINTS_LIBRARY: "BlueprintsLibrary",
    MESSANGER: "Messanger",
} as const;

export type ModuleType = typeof ModuleType[keyof typeof ModuleType];

export type MidlevelModule =
    | Ship
    | SystemClock
    | Engine
    | ResourceContainer
    | CelestialScanner
    | PassiveScanner
    | AsteroidScanner
    | AsteroidMiner
    | Shipyard
    | BlueprintsLibrary
    | Messanger;

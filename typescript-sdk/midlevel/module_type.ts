
// Server reports a ship as "Ship/<subtype>", every other module type is
// reported as is.
export const ModuleType = {
    SHIP: "Ship/",
    SYSTEM_CLOCK: "SystemClock",
    ENGINE: "Engine",
    RESOURCE_CONTAINER: "ResourceContainer",
    PASSIVE_SCANNER: "PassiveScanner",
    ASTEROID_SCANNER: "AsteroidScanner",
    ASTEROID_MINER: "AsteroidMiner",
    SHIPYARD: "Shipyard",
    BLUEPRINTS_LIBRARY: "BlueprintsLibrary",
    MESSANGER: "Messanger",
} as const;

export type ModuleType = typeof ModuleType[keyof typeof ModuleType];

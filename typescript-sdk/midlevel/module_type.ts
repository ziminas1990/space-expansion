
// Runtime module_type is the module type ("Ship", "RCS", ...).
export const ModuleType = {
    SHIP: "Ship",
    SYSTEM_CLOCK: "SystemClock",
    RCS: "RCS",
    RESOURCE_CONTAINER: "ResourceContainer",
    PASSIVE_SCANNER: "PassiveScanner",
    ASTEROID_SCANNER: "AsteroidScanner",
    ASTEROID_MINER: "AsteroidMiner",
    SHIPYARD: "Shipyard",
    BLUEPRINTS_LIBRARY: "BlueprintsLibrary",
    MESSANGER: "Messanger",
    GAME: "Game",
} as const;

export type ModuleType = typeof ModuleType[keyof typeof ModuleType];

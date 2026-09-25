import { AsteroidMiner } from "./asteroid_miner.js";
import { AsteroidScanner } from "./asteroid_scanner.js";
import { BlueprintsLibrary } from "./blueprints_library.js";
import { HoverEngine } from "./hover_engine.js";
import { RCS } from "./rcs.js";
import { Game } from "./game.js";
import { Messanger } from "./messanger.js";
import { ModuleType } from "./module_type.js";
import { PassiveScanner } from "./passive_scanner.js";
import { ResourceContainer } from "./resource_container.js";
import { Ship } from "./ship.js";
import { Shipyard } from "./shipyard.js";
import { SystemClock } from "./system_clock.js";

// TODO: remove re-export?
export * from "./module_type.js";

export type MidlevelModule =
    | Ship
    | SystemClock
    | RCS
    | HoverEngine
    | ResourceContainer
    | PassiveScanner
    | AsteroidScanner
    | AsteroidMiner
    | Shipyard
    | BlueprintsLibrary
    | Messanger
    | Game;

export function is_module<T extends ModuleType>(
    module: MidlevelModule,
    type: T,
): module is Extract<MidlevelModule, { type: T }> {
    return module.type === type;
}

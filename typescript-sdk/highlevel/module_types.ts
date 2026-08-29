import { AsteroidMiner } from "./asteroid_miner.js";
import { AsteroidScanner } from "./asteroid_scanner.js";
import { BlueprintsLibrary } from "./blueprints_library.js";
import { Engine } from "./engine.js";
import { Messanger } from "./messanger.js";
import { PassiveScanner } from "./passive_scanner.js";
import { ResourceContainer } from "./resource_container.js";
import { Ship } from "./ship.js";
import { Shipyard } from "./shipyard.js";
import { SystemClock } from "./system_clock.js";

// TODO: get rid of re-export
export { ModuleType } from "../midlevel/module_types.js";

export type HighlevelModule =
    | Ship
    | SystemClock
    | Engine
    | ResourceContainer
    | PassiveScanner
    | AsteroidScanner
    | AsteroidMiner
    | Shipyard
    | BlueprintsLibrary
    | Messanger;

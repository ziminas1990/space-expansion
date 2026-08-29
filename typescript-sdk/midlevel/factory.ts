import { OpenSessionCallback } from "./base_module.js";
import { AsteroidMiner } from "./asteroid_miner.js";
import { AsteroidScanner } from "./asteroid_scanner.js";
import { BlueprintsLibrary } from "./blueprints_library.js";
import { Engine } from "./engine.js";
import { Messanger } from "./messanger.js";
import { MidlevelModule, ModuleType } from "./module_types.js";
import { PassiveScanner } from "./passive_scanner.js";
import { ResourceContainer } from "./resource_container.js";
import { Ship } from "./ship.js";
import { Shipyard } from "./shipyard.js";
import { SystemClock } from "./system_clock.js";

type ModuleConstructor = new (open_session_cb: OpenSessionCallback) => MidlevelModule;

const module_constructors: Record<string, ModuleConstructor> = {
    [ModuleType.SYSTEM_CLOCK]: SystemClock,
    [ModuleType.ENGINE]: Engine,
    [ModuleType.RESOURCE_CONTAINER]: ResourceContainer,
    [ModuleType.PASSIVE_SCANNER]: PassiveScanner,
    [ModuleType.ASTEROID_SCANNER]: AsteroidScanner,
    [ModuleType.ASTEROID_MINER]: AsteroidMiner,
    [ModuleType.SHIPYARD]: Shipyard,
    [ModuleType.BLUEPRINTS_LIBRARY]: BlueprintsLibrary,
    [ModuleType.MESSANGER]: Messanger,
};

// Returns undefined for interfaces that are not attached to a slot, such as
// INavigation and IGame.
export function create_module(
    module_type: string,
    open_session_cb: OpenSessionCallback): MidlevelModule | undefined
{
    if (module_type.startsWith(ModuleType.SHIP)) {
        return new Ship(open_session_cb);
    }
    const constructor = module_constructors[module_type];
    return constructor ? new constructor(open_session_cb) : undefined;
}

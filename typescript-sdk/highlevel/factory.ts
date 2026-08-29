import * as midlevel from "../midlevel/index.js";
import { AsteroidMiner } from "./asteroid_miner.js";
import { AsteroidScanner } from "./asteroid_scanner.js";
import type { BaseModule } from "./base_module.js";
import { BlueprintsLibrary } from "./blueprints_library.js";
import { CelestialScanner } from "./celestial_scanner.js";
import { Engine } from "./engine.js";
import { Messanger } from "./messanger.js";
import { HighlevelModule, ModuleType } from "./module_types.js";
import { PassiveScanner } from "./passive_scanner.js";
import { ResourceContainer } from "./resource_container.js";
import { Ship } from "./ship.js";
import { Shipyard } from "./shipyard.js";
import { SystemClock } from "./system_clock.js";

export type SlotInfo = {
    module_type: string;
    module_name: string;
};

export type CreateModule = (
    info: SlotInfo,
    midlevel_module: midlevel.MidlevelModule,
) => HighlevelModule | undefined;

function wrap<M extends midlevel.MidlevelModule, H extends BaseModule>(
    instance: midlevel.MidlevelModule,
    Mid: new (...args: any[]) => M,
    High: new (rpc: M, name: string) => H,
    name: string,
): H | undefined {
    return instance instanceof Mid ? new High(instance, name) : undefined;
}

// Wraps a midlevel client created by ModuleRegistry for a commutator slot.
// Returns undefined for interfaces that are not attached to a slot, such as
// INavigation and IGame.
export function create_module(
    info: SlotInfo,
    midlevel_module: midlevel.MidlevelModule,
): HighlevelModule | undefined {
    if (info.module_type.startsWith(ModuleType.SHIP)) {
        if (!(midlevel_module instanceof midlevel.Ship)) {
            return undefined;
        }
        return new Ship(midlevel_module, info.module_name, info.module_type, create_module);
    }

    switch (info.module_type) {
        case ModuleType.SYSTEM_CLOCK:
            return wrap(midlevel_module, midlevel.SystemClock, SystemClock, info.module_name);
        case ModuleType.ENGINE:
            return wrap(midlevel_module, midlevel.Engine, Engine, info.module_name);
        case ModuleType.RESOURCE_CONTAINER:
            return wrap(
                midlevel_module, midlevel.ResourceContainer, ResourceContainer, info.module_name);
        case ModuleType.CELESTIAL_SCANNER:
            return wrap(
                midlevel_module, midlevel.CelestialScanner, CelestialScanner, info.module_name);
        case ModuleType.PASSIVE_SCANNER:
            return wrap(
                midlevel_module, midlevel.PassiveScanner, PassiveScanner, info.module_name);
        case ModuleType.ASTEROID_SCANNER:
            return wrap(
                midlevel_module, midlevel.AsteroidScanner, AsteroidScanner, info.module_name);
        case ModuleType.ASTEROID_MINER:
            return wrap(
                midlevel_module, midlevel.AsteroidMiner, AsteroidMiner, info.module_name);
        case ModuleType.SHIPYARD:
            return wrap(midlevel_module, midlevel.Shipyard, Shipyard, info.module_name);
        case ModuleType.BLUEPRINTS_LIBRARY:
            return wrap(
                midlevel_module, midlevel.BlueprintsLibrary, BlueprintsLibrary, info.module_name);
        case ModuleType.MESSANGER:
            return wrap(midlevel_module, midlevel.Messanger, Messanger, info.module_name);
        default:
            return undefined;
    }
}

import { HoverEngine } from "./hover_engine.js";
import { InstalledModule, type InstalledModulePacked, type ModuleInfo } from "./module.js";
import { RCS } from "./rcs.js";
import { ResourceContainer } from "./resource_container.js";
import { Shipyard } from "./shipyard.js";

export function create_module(info: ModuleInfo): InstalledModule {
    switch (info.type) {
        case "ResourceContainer":
            return new ResourceContainer(info.slot_id, info.name);
        case "HoverEngine":
            return new HoverEngine(info.slot_id, info.name);
        case "RCS":
            return new RCS(info.slot_id, info.name);
        case "Shipyard":
            return new Shipyard(info.slot_id, info.name);
        default:
            return new InstalledModule(info.slot_id, info.type, info.name);
    }
}

export function unpack_module([slot_id, type, name, snapshot]: InstalledModulePacked)
    : InstalledModule {
    const module = create_module({ slot_id, type, name });
    if (snapshot === undefined) {
        return module;
    }
    if (module instanceof ResourceContainer && "resources" in snapshot) {
        module.update_content(snapshot);
    } else if (module instanceof HoverEngine && "thrust" in snapshot) {
        module.update_thrust(snapshot.thrust);
    } else if (module instanceof RCS && "direction" in snapshot) {
        module.update_thrust(snapshot);
    } else if (module instanceof Shipyard && "status" in snapshot) {
        module.update_state(snapshot);
    }
    return module;
}

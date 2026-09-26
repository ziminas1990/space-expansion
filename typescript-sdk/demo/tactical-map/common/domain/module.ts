import type { ContainerContent } from "./resource_container.js";
import type { RCSThrust } from "./rcs.js";
import type { ShipyardState } from "./shipyard.js";

export type ModuleInfo = {
    slot_id: number;
    type: string;
    name: string;
};

export type ModuleSnapshot = ContainerContent | { thrust: number } | RCSThrust | ShipyardState;
export type InstalledModulePacked = [number, string, string, ModuleSnapshot?];
export type ModuleInfoPacked = [number, string, string];

export class InstalledModule implements ModuleInfo {
    constructor(
        readonly slot_id: number,
        readonly type: string,
        readonly name: string,
    ) {}

    pack(): InstalledModulePacked {
        return [this.slot_id, this.type, this.name];
    }
}

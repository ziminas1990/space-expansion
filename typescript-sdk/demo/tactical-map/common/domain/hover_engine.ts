import { InstalledModule, type InstalledModulePacked } from "./module.js";

export class HoverEngine extends InstalledModule {
    private thrust?: number;

    constructor(slot_id: number, name: string) {
        super(slot_id, "HoverEngine", name);
    }

    get_thrust(): number | undefined {
        return this.thrust;
    }

    update_thrust(thrust: number): void {
        this.thrust = thrust;
    }

    override pack(): InstalledModulePacked {
        return this.thrust === undefined
            ? super.pack()
            : [this.slot_id, this.type, this.name, { thrust: this.thrust }];
    }
}

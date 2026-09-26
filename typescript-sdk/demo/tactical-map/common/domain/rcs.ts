import { InstalledModule, type InstalledModulePacked } from "./module.js";
import type { Vector2D } from "./position.js";

export type RCSThrust = {
    thrust: number;
    direction: Vector2D;
};

export class RCS extends InstalledModule {
    private thrust?: RCSThrust;

    constructor(slot_id: number, name: string) {
        super(slot_id, "RCS", name);
    }

    get_thrust(): RCSThrust | undefined {
        return this.thrust === undefined
            ? undefined
            : { thrust: this.thrust.thrust, direction: { ...this.thrust.direction } };
    }

    update_thrust(thrust: RCSThrust): void {
        this.thrust = { thrust: thrust.thrust, direction: { ...thrust.direction } };
    }

    override pack(): InstalledModulePacked {
        return this.thrust === undefined
            ? super.pack()
            : [this.slot_id, this.type, this.name, this.get_thrust()!];
    }
}

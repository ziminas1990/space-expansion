import { InstalledModule, type InstalledModulePacked } from "./module.js";

export type ShipyardState =
    | { status: "idle" }
    | {
        status: "building" | "frozen";
        blueprint_name: string;
        ship_name: string;
        progress: number;
    };

export class Shipyard extends InstalledModule {
    private state?: ShipyardState;

    constructor(slot_id: number, name: string) {
        super(slot_id, "Shipyard", name);
    }

    get_state(): ShipyardState | undefined {
        return this.state === undefined ? undefined : { ...this.state };
    }

    update_state(state: ShipyardState): void {
        this.state = state.status === "idle"
            ? { status: "idle" }
            : {
                ...state,
                progress: Number.isFinite(state.progress)
                    ? Math.max(0, Math.min(1, state.progress))
                    : 0,
            };
    }

    override pack(): InstalledModulePacked {
        return this.state === undefined
            ? super.pack()
            : [this.slot_id, this.type, this.name, { ...this.state }];
    }
}

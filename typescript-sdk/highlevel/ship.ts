import * as midlevel from "../midlevel/index.js";
import { Status } from "../types/index.js";

export type ShipState = midlevel.ShipState

export class Ship {
    private state: ShipState | undefined = undefined;

    private monitor: Promise<Status> | undefined = undefined;
    private stop_monitoring: boolean = false;

    constructor(private rpc: midlevel.Ship, public name: string)
    {}

    async init(): Promise<Status> {
        // Start monitoring of commutator's state
        this.stop_monitoring = false;
        this.monitor = new Promise(async (resolve) => {
            resolve(await this.monitoring());
        });
        return Status.ok();
    }

    async reinit(rpc: midlevel.Ship): Promise<Status> {
        await this.release();
        await this.rpc.terminate();
        this.rpc = rpc;
        return await this.init();
    }

    async release(): Promise<Status> {
        this.stop_monitoring = true;
        if (this.monitor) {
            await this.monitor;
        }
        return Status.ok();
    }

    private handle_update(update: ShipState) {
        if (this.state == undefined) {
            this.state = update;
            return;
        }

        if (update.position) {
            this.state.position = update.position;
        }
        if (update.weight) {
            this.state.weight = update.weight;
        }
    }

    private async monitoring(): Promise<Status> {
        while (!this.stop_monitoring) {
            // Get all modules info
            const [status, state] = await this.rpc.get_state();
            if (status.is_ok()) {
                this.state = state;
                // Subscribe for updates
                await this.rpc.monitoring(100, async (status, state) => {
                    if (status.is_ok() && state) {
                        this.handle_update(state);
                    }
                    return !this.stop_monitoring;
                });
                if (this.stop_monitoring) {
                    return Status.ok();
                }
            }
            // Something went wrong, try start monitoring again after 200ms
            await new Promise((r) => setTimeout(r, 200));
        }
        return Status.ok();
    }

}
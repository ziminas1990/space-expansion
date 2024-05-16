import * as midlevel from "../midlevel/index.js";
import { Status } from "../types/status.js";

export type ModuleInfo = midlevel.ModuleInfo;
export type CommutatorUpdate = midlevel.CommutatorUpdate;

export type Events = {
    reset: ((module: ModuleInfo[]) => Promise<void>);
    attached: ((module: ModuleInfo) => Promise<void>);
    detached: ((module: ModuleInfo) => Promise<void>);
}

export class Commutator {
    public modules: ModuleInfo[] = [];
    private monitor: Promise<Status> | undefined = undefined;
    private stop_monitoring: boolean = false;

    private listeners: { [K in keyof Events]: Events[K][] } = {
        reset: [],
        attached: [],
        detached: [],
    };

    constructor(private rpc: midlevel.Commutator) {}

    on<K extends keyof Events>(event: K, listener: Events[K]): Status {
        if (this.listeners[event] != undefined) {
            this.listeners[event]!.push(listener);
            return Status.ok();
        }
        return Status.fail(`invalid event ${event}`);
    }

    async init(): Promise<Status> {
        // Start monitoring of commutator's state
        this.stop_monitoring = true;
        this.monitor = new Promise(async (resolve) => {
            while (true) {
                // Get all modules info
                const fetch_status = await this.fetch_all_modules_info();
                if (fetch_status.is_ok()) {
                    // Subscribe for updates
                    await this.rpc.monitoring(async (status, update) => {
                        if (status.is_ok() && update) {
                            this.handle_update(update);
                        }
                        return !this.stop_monitoring;
                    });
                    if (this.stop_monitoring) {
                        resolve(Status.ok());
                    }
                }
                // Something went wrong, try start monitoring again in a second
                await new Promise((r) => setTimeout(r, 1000));
            }
        });
        return Status.ok();
    }

    async release(): Promise<Status> {
        if (this.monitor) {
            this.stop_monitoring = true;
            await this.monitor;
        }
        return Status.ok();
    }

    async open_session(slot_id: number): Promise<[Status, midlevel.Session | undefined]> {
        return await this.rpc.open_tunnel(slot_id);
    }

    async close_session(session_id: number): Promise<Status> {
        return await this.rpc.close_tunnel(session_id);
    }

    private async fetch_all_modules_info(): Promise<Status> {
        const [status, modules] = await this.rpc.get_all_modules_info();
        if (!status.is_ok()) {
            return status.wrap("failed to get all modules info");
        }
        this.modules = modules || [];
        await this.emit("reset", this.modules);
        this.modules.forEach(async (module) => {
            await this.emit("attached", module);
        })
        return Status.ok();
    }

    private async handle_update(update: CommutatorUpdate): Promise<void> {
        if (update.module_attached) {
            this.modules.push(update.module_attached);
            await this.emit("attached", update.module_attached);
        }
        if (update.module_detached) {
            const index = this.modules.findIndex((m) => m.slot_id == update.module_detached);
            if (index >= 0) {
                const [module] = this.modules.splice(index, 1);
                await this.emit("detached", module!);
            }
        }
    }

    private async emit<K extends keyof Events>(event: K, ...params: Parameters<Events[K]>):
    Promise<void>
    {
        this.listeners[event].forEach(async (listener) => {
            await (listener as (...args: Parameters<Events[K]>) => any)(...params)
        });
    }

}
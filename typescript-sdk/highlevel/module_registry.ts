import * as midlevel from "../midlevel/index.js";
import { ModuleType } from "../midlevel/module_types.js";
import { Status } from "../types/status.js";
import { EventEmitter } from "./events.js";
import type { BaseModule } from "./base_module.js";
import type { CreateModule } from "./factory.js";
import type { HighlevelModule } from "./module_types.js";

export type { CreateModule };

export type RegisteredSlot = {
    module_type: string;
    module_name: string;
    module: HighlevelModule;
};

export type Events = {
    attached: (module: HighlevelModule) => Promise<void> | void;
    detached: (module: HighlevelModule) => Promise<void> | void;
};

type Initable = HighlevelModule & {
    init: () => Promise<Status>;
};

function identity_key(module_type: string, module_name: string): string {
    const type = module_type.startsWith(ModuleType.SHIP)
        ? ModuleType.SHIP
        : module_type;
    return `${type}::${module_name}`;
}

function has_init(module: HighlevelModule): module is Initable {
    return "init" in module
        && typeof (module as { init?: unknown }).init === "function";
}

export class ModuleRegistry extends EventEmitter<Events> {
    // slot_id -> slot info
    public readonly slots = new Map<number, RegisteredSlot>();

    // The `attached` map has only modules that are currently attached to the
    // commutator. When a module detaches, it is removed from the map.
    // type -> name -> module
    public readonly attached = new Map<ModuleType, Map<string, HighlevelModule>>();

    // The `known` map has all modules that were ever attached to the
    // commutator. When a module detaches, it is removed from the `attached` map
    // but kept in the `known` map (after release()). If later a module with the
    // same type and name is attached again, the object from `known` is reused
    // as the highlevel representation (reinit() is called on it).
    // So if client code once got a module, then the module was detached and
    // reattached later, the client may keep using the same object. The object
    // is not usable in the gap between release() and reinit()
    // `type::name` -> module
    private readonly known = new Map<string, HighlevelModule>();

    private monitor: Promise<void> | undefined = undefined;
    private stop_monitoring = false;

    constructor(
        private rpc: midlevel.Commutator,
        // This MUST be passed as a callback instead of being imported in order
        // to avoid circular dependencies:
        // factory -> ship -> module_registry -> factory
        private readonly create_module: CreateModule,
    ) {
        super();
    }

    down_level(): midlevel.Commutator {
        return this.rpc;
    }

    async init(): Promise<Status> {
        this.stop_monitoring = false;
        const status = await this.synchronize_slots();
        if (!status.is_ok()) {
            return status;
        }
        this.monitor = this.monitoring_loop();
        return Status.ok();
    }

    get_all<T extends ModuleType>(
        type: T,
    ): Extract<HighlevelModule, { type: T }>[] {
        const by_name = this.attached.get(type);
        if (!by_name) {
            return [];
        }
        return [...by_name.values()] as Extract<HighlevelModule, { type: T }>[];
    }

    get_by_name<T extends ModuleType>(
        type: T,
        name: string,
    ): Extract<HighlevelModule, { type: T }> | undefined {
        return this.attached.get(type)?.get(name) as
            | Extract<HighlevelModule, { type: T }>
            | undefined;
    }

    async release(): Promise<Status> {
        this.stop_monitoring = true;
        const wrappers = new Set<BaseModule>(this.known.values());
        for (const slot of this.slots.values()) {
            wrappers.add(slot.module);
        }
        await this.rpc.terminate();
        if (this.monitor) {
            await this.monitor;
            this.monitor = undefined;
        }
        try {
            await Promise.all([...wrappers].map((module) => module.release()));
        } catch (_error) {
            // best effort. release() should never throw, but just in case...
        }
        this.slots.clear();
        this.attached.clear();
        this.known.clear();
        return Status.ok();
    }

    private spawn_client(info: midlevel.ModuleInfo): midlevel.MidlevelModule | undefined {
        return midlevel.create_module(info.module_type, info.open_session_cb);
    }

    private async monitoring_loop(): Promise<void> {
        while (!this.stop_monitoring) {
            const status = await this.rpc.monitoring(async (update) => {
                if (update) {
                    await this.handle_update(update);
                }
                return !this.stop_monitoring;
            });
            if (!status.is_ok()) {
                break;
            }
        }
    }

    private async synchronize_slots(): Promise<Status> {
        const [status, snapshot] = await this.rpc.get_all_modules_info();
        if (!status.is_ok()) {
            return status.wrap("failed to get all modules info");
        }
        await this.diff_slots(snapshot || []);
        return Status.ok();
    }

    private async diff_slots(snapshot: midlevel.ModuleInfo[]): Promise<void> {
        const incoming = new Set(snapshot.map((info) => info.slot_id));
        for (const slot_id of [...this.slots.keys()]) {
            if (!incoming.has(slot_id)) {
                await this.detach_slot(slot_id);
            }
        }
        for (const info of snapshot) {
            await this.attach_slot(info);
        }
    }

    private async handle_update(update: midlevel.CommutatorUpdate): Promise<void> {
        if (update.module_detached !== undefined) {
            await this.detach_slot(update.module_detached);
        }
        if (update.module_attached) {
            await this.attach_slot(update.module_attached);
        }
    }

    private async attach_slot(info: midlevel.ModuleInfo): Promise<void> {
        const current = this.slots.get(info.slot_id);
        if (current
            && current.module_type === info.module_type
            && current.module_name === info.module_name)
        {
            return;
        }
        if (current) {
            await this.detach_slot(info.slot_id);
        }

        const rpc = this.spawn_client(info);
        if (!rpc) {
            return;
        }

        const key = identity_key(info.module_type, info.module_name);
        const known = this.known.get(key);
        let module: HighlevelModule;
        if (known) {
            await known.reinit(rpc);
            module = known;
        } else {
            const created = this.create_module(info, rpc);
            if (!created) {
                await rpc.terminate();
                return;
            }
            if (has_init(created)) {
                await created.init();
            }
            this.known.set(key, created);
            module = created;
        }

        this.slots.set(info.slot_id, {
            module_type: info.module_type,
            module_name: info.module_name,
            module,
        });
        this.index(module);
        await this.emit("attached", module);
    }

    private async detach_slot(slot_id: number): Promise<void> {
        const registered = this.slots.get(slot_id);
        if (!registered) {
            return;
        }
        this.slots.delete(slot_id);
        this.unindex(registered.module);
        await registered.module.release();
        await this.emit("detached", registered.module);
    }

    private index(module: HighlevelModule): void {
        let by_name = this.attached.get(module.type);
        if (!by_name) {
            by_name = new Map();
            this.attached.set(module.type, by_name);
        }
        by_name.set(module.name, module);
    }

    private unindex(module: HighlevelModule): void {
        const by_name = this.attached.get(module.type);
        if (by_name?.get(module.name) === module) {
            by_name.delete(module.name);
            if (by_name.size === 0) {
                this.attached.delete(module.type);
            }
        }
    }
}

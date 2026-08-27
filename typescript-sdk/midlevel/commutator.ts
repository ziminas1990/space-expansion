import * as lowlevel from "../lowlevel/index.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { create_module } from "./factory.js";
import { MidlevelModule } from "./module_types.js";
import { Status } from "../types/status.js";

export type ModuleInfo = lowlevel.ModuleInfo & {
    open_session_cb: OpenSessionCallback;
}

export type Update = {
    module_attached?: ModuleInfo;
    module_detached?: number;
}

export type MonitoringCallback =
    (status: Status, update: Update | undefined) => Promise<boolean>;

export type Session = lowlevel.Session;

export type RegisteredSlot = {
    module_type: string;
    module_name: string;
    module: MidlevelModule;
}

export class Commutator extends BaseModule<lowlevel.Commutator> {
    // slot_id -> solt info
    public readonly slots: Map<number, RegisteredSlot> = new Map();
    // type -> name -> module
    public readonly modules: Map<string, Map<string, MidlevelModule>> = new Map();

    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Commutator(session)]);
    }

    async total_slots(): Promise<[Status, number | undefined]> {
        return await this.run(this._total_slots);
    }

    async get_slot_info(slot_id: number): Promise<[Status, ModuleInfo | undefined]> {
        return await this.run(async (session) => this._get_slot_info(session, slot_id));
    }

    async get_all_modules_info(): Promise<[Status, ModuleInfo[] | undefined]> {
        const [status, modules] = await this.run(
            async (session) => this._get_all_modules_info(session));
        if (status.is_ok() && modules) {
            await this.synchronize_modules(modules);
        }
        return [status, modules];
    }

    async open_tunnel(slot_id: number): Promise<[Status, lowlevel.Session | undefined]> {
        return await this.run(async (session) => this._open_session(session, slot_id));
    }

    async close_tunnel(session_id: number): Promise<Status> {
        return await this.run_no_return(async (session) => this._close_session(session, session_id));
    }

    async monitoring(callback: MonitoringCallback)
    : Promise<[Status, unknown]>
    {
        return await this.run(async (session) => this._monitoring(session, callback), true);
    }

    override async terminate(): Promise<void> {
        const slot_ids = Array.from(this.slots.keys());
        for (const slot_id of slot_ids) {
            await this.detach_module(slot_id);
        }
        await super.terminate();
    }

    private async _total_slots(
        session: lowlevel.Commutator,
        timeout_ms: number = 500): Promise<[Status, number]>
    {
        const send_status = await session.send_total_slots_request();
        if (!send_status.is_ok()) {
            return [send_status, 0];
        }
        return await session.wait_total_slots_response(timeout_ms);
    }

    private async _get_slot_info(
        session: lowlevel.Commutator,
        slot_id: number): Promise<[Status, ModuleInfo | undefined]>
    {
        const send_status = await session.send_module_info_request(slot_id);
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, info] = await session.wait_module_info_response();
        if (!status.is_ok() || !info) {
            return [status.wrap(`can't get info for slot ${slot_id}`), undefined];
        }
        return [Status.ok(), {
            ...info,
            open_session_cb: async () => this.open_tunnel(slot_id)
        }];
    }

    private async _get_all_modules_info(
        session: lowlevel.Commutator): Promise<[Status, ModuleInfo[] | undefined]>
    {
        const [status, total] = await this._total_slots(session);
        if (!status.is_ok()) {
            return [status.wrap("can't get total slots count"), undefined];
        }

        const send_status = await session.send_all_modules_info_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }

        const modules_info: ModuleInfo[] = [];
        for (let i = 0; i < total; i++) {
            const [info_status, info] = await session.wait_module_info_response();
            if (!info_status.is_ok() || !info) {
                return [info_status.wrap(`can't get info for module ${i}`), modules_info];
            }
            if (info) {
                modules_info.push({
                    ...info,
                    open_session_cb: async () => this.open_tunnel(info.slot_id)
                });
            }
        }
        return [Status.ok(), modules_info];
    }

    private async _open_session(commutator: lowlevel.Commutator, slot_id: number)
    : Promise<[Status, lowlevel.Session | undefined]>
    {
        const send_status = await commutator.send_open_tunnel_request(slot_id);
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, tunnel_id] = await commutator.wait_open_tunnel_report();
        if (!status.is_ok() || tunnel_id == undefined) {
            return [
                status.wrap(`can't open tunnel to slot ${slot_id}`),
                undefined
            ];
        }
        if (tunnel_id == 0) {
            return [Status.fail(`reject from server`), undefined];
        }

        const [reg_status, new_session] = commutator.enable_tunnel(tunnel_id);
        if (!reg_status.is_ok() || !new_session) {
            return [
                reg_status.wrap(`can't register new session ${tunnel_id}`),
                undefined
            ];
        }
        return [Status.ok(), new_session];
    }

    private async _close_session(session: lowlevel.Commutator, session_id: number)
        : Promise<Status>
    {
        const send_status = await session.send_close_tunnel_request(session_id);
        if (!send_status.is_ok()) {
            return send_status;
        }
        return await session.wait_close_tunnel_status();
    }

    private async _monitoring(
        session: lowlevel.Commutator, callback: MonitoringCallback)
    :Promise<[Status, unknown]>
    {
        const send_status = await session.send_start_monitoring_request();
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to start monitoring"), undefined];
        }

        {
            const status = await session.wait_monitor_ack();
            if (!status.is_ok()) {
                return [status, undefined];
            }
        }

        while (true) {
            const [status, update] = await session.wait_update();
            if (!status.is_ok()) {
                const stop_status = status.wrap("monitoring stopped");
                callback(stop_status, undefined);
                return [stop_status, undefined];
            }
            if (update) {
                const public_update: Update = {
                    module_attached: update.module_attached ? {
                        ...update.module_attached,
                        open_session_cb: async () => this.open_tunnel(
                            update.module_attached!.slot_id)
                    } : undefined,
                    module_detached: update.module_detached
                };
                await this.apply_update(public_update);
                const resume = await callback(Status.ok(), public_update);
                if (!resume) {
                    return [Status.ok(), undefined];
                }
            } else {
                const resume = await callback(Status.ok(), undefined);
                if (!resume) {
                    return [Status.ok(), undefined];
                }
            }
        }
    }

    private async synchronize_modules(modules: ModuleInfo[]): Promise<void> {
        const current_slots = new Map(
            modules.map((module) => [module.slot_id, module]));

        for (const [slot_id, registered] of this.slots) {
            const current = current_slots.get(slot_id);
            if (!current
                || current.module_type != registered.module_type
                || current.module_name != registered.module_name)
            {
                await this.detach_module(slot_id);
            }
        }

        for (const module of modules) {
            const registered = this.slots.get(module.slot_id);
            if (!registered) {
                await this.attach_module(module);
            }
        }
    }

    private async apply_update(update: Update): Promise<void> {
        if (update.module_attached) {
            await this.attach_module(update.module_attached);
        }
        if (update.module_detached !== undefined) {
            await this.detach_module(update.module_detached);
        }
    }

    private async attach_module(info: ModuleInfo): Promise<void> {
        const registered = this.slots.get(info.slot_id);
        if (registered
            && registered.module_type == info.module_type
            && registered.module_name == info.module_name)
        {
            return;
        }
        if (registered) {
            await this.detach_module(info.slot_id);
        }

        const module = create_module(info.module_type, info.open_session_cb);
        if (!module) {
            return;
        }

        this.slots.set(info.slot_id, {
            module_type: info.module_type,
            module_name: info.module_name,
            module,
        });
        let modules_by_name = this.modules.get(info.module_type);
        if (!modules_by_name) {
            modules_by_name = new Map();
            this.modules.set(info.module_type, modules_by_name);
        }
        modules_by_name.set(info.module_name, module);
    }

    private async detach_module(slot_id: number): Promise<void> {
        const registered = this.slots.get(slot_id);
        if (!registered) {
            return;
        }

        this.slots.delete(slot_id);
        const modules_by_name = this.modules.get(registered.module_type);
        if (modules_by_name?.get(registered.module_name) == registered.module) {
            modules_by_name.delete(registered.module_name);
            if (modules_by_name.size == 0) {
                this.modules.delete(registered.module_type);
            }
        }
        await registered.module.terminate();
    }
}
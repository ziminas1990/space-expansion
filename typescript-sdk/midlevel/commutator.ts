import * as lowlevel from "../lowlevel/index.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { Status } from "../types/status.js";

export type ModuleInfo = lowlevel.ModuleInfo;
export type CommutatorUpdate = lowlevel.CommutatorUpdate;
export type MonitoringCallback =
    (status: Status, update: CommutatorUpdate | undefined) => Promise<boolean>;

export class Commutator extends BaseModule<lowlevel.Commutator> {
    constructor(open_session_callback: OpenSessionCallback,
                private root_session: lowlevel.RootSession)
    {
        super(open_session_callback,
              async (session) => this.create_interface(session));
    }

    async total_slots(): Promise<[Status, number | undefined]> {
        return await this.run(this._total_slots);
    }

    private async create_interface(session: lowlevel.Session): Promise<[Status, lowlevel.Commutator]> {
        return [Status.ok(), new lowlevel.Commutator(session)];
    }

    async get_slot_info(slot_id: number): Promise<[Status, ModuleInfo | undefined]> {
        return await this.run(async (session) => this._get_slot_info(session, slot_id));
    }

    async get_all_modules_info(): Promise<[Status, ModuleInfo[] | undefined]> {
        return await this.run(async (session) => this._get_all_modules_info(session));
    }

    async open_tunnel(slot_id: number): Promise<[Status, lowlevel.Session | undefined]> {
        return await this.run(async (session) => this._open_session(session, slot_id));
    }

    async close_tunnel(session_id: number): Promise<[Status, unknown]> {
        return await this.run(async (session) => this._close_session(session, session_id));
    }

    async monitoring(callback: MonitoringCallback)
    : Promise<[Status, unknown]>
    {
        return await this.run(async (session) => this._monitoring(session, callback), true);
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
        return await session.wait_module_info_response(slot_id);
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
            const [info_status, info] = await session.wait_module_info_response(i);
            if (!info_status.is_ok() || !info) {
                return [info_status.wrap(`can't get info for module ${i}`), modules_info];
            }
            if (info) {
                modules_info.push(info);
            }
        }
        return [Status.ok(), modules_info];
    }

    private async _open_session(session: lowlevel.Commutator, slot_id: number)
    : Promise<[Status, lowlevel.Session | undefined]>
    {
        const send_status = await session.send_open_tunnel_request(slot_id);
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, session_id] = await session.wait_open_tunnel_report(slot_id);
        if (!status.is_ok() || session_id == undefined) {
            return [status.wrap(`can't open tunnel to slot ${slot_id}`), undefined];
        }
        if (session_id == 0) {
            return [Status.fail(`reject from server`), undefined];
        }

        const [reg_status, new_session] = this.root_session.register_new_session(session_id);
        if (!reg_status.is_ok() || !new_session) {
            return [reg_status.wrap(`can't register new session ${session_id}`), undefined];
        }
        return [Status.ok(), new_session];
    }

    private async _close_session(session: lowlevel.Commutator, session_id: number)
        : Promise<[Status, unknown]>
    {
        const send_status = await session.send_close_tunnel_request(session_id);
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        return [await session.wait_close_tunnel_status(), undefined];
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
            } else if (update) {
                const resume = await callback(Status.ok(), update);
                if (!resume) {
                    return [Status.ok(), undefined];
                }
            }
        }
    }
}
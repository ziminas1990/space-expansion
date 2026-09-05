import * as lowlevel from "#sdk/lowlevel/index.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { Status } from "#sdk/types/status.js";

export type ModuleInfo = lowlevel.ModuleInfo & {
    open_session_cb: OpenSessionCallback;
}

export type Update = {
    module_attached?: ModuleInfo;
    module_detached?: number;
}

export type MonitoringCallback =
    (update: Update | undefined) => Promise<boolean>;

export type Session = lowlevel.Session;

export class Commutator extends BaseModule<lowlevel.Commutator> {

    constructor(
        open_session_callback: OpenSessionCallback,
        private readonly terminate_root?: () => Promise<unknown>,
    )
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
        return await this.run(
            async (session) => this._get_all_modules_info(session));
    }

    async open_tunnel(slot_id: number): Promise<[Status, lowlevel.Session | undefined]> {
        return await this.run(async (session) => this._open_session(session, slot_id));
    }

    async close_tunnel(session_id: number): Promise<Status> {
        return await this.run_no_return(async (session) => this._close_session(session, session_id));
    }

    async monitoring(callback: MonitoringCallback)
    : Promise<Status>
    {
        return await this.run_no_return(async (session) => this._monitoring(session, callback), true);
    }

    override async terminate(): Promise<void> {
        await super.terminate();
        // It is expected that only RootCommutator has a terminate_root
        // callback. It closes the connection to the server.
        await this.terminate_root?.();
    }

    private bind_info(info: lowlevel.ModuleInfo): ModuleInfo {
        return {
            ...info,
            open_session_cb: async () => this.open_tunnel(info.slot_id)
        };
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
        return [Status.ok(), this.bind_info(info)];
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
                modules_info.push(this.bind_info(info));
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
    : Promise<Status>
    {
        const send_status = await session.send_start_monitoring_request();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to start monitoring");
        }

        {
            const status = await session.wait_monitor_ack();
            if (!status.is_ok()) {
                return status;
            }
        }

        while (true) {
            const [status, update] = await session.wait_update();
            if (status.is_timeout()) {
                // Just a heartbeat for upper level, so that it could have a
                // chance to report that monitoring should be stopped by
                // returning false value.
                const resume = await callback(undefined);
                if (!resume) {
                    return Status.ok();
                }
                continue;
            }
            if (!status.is_ok()) {
                return status.wrap("monitoring stopped");
            }
            const public_update: Update | undefined = update
                ? {
                    module_attached: update.module_attached
                        ? this.bind_info(update.module_attached)
                        : undefined,
                    module_detached: update.module_detached
                }
                : undefined;
            const resume = await callback(public_update);
            if (!resume) {
                return Status.ok();
            }
        }
    }
}

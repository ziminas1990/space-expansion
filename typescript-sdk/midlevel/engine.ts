import * as lowlevel from "#sdk/lowlevel/index.js";
import { Status } from "#sdk/types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type EngineSpecification = lowlevel.EngineSpecification;
export type CurrentThrust = lowlevel.CurrentThrust;
export type MonitoringCallback =
    (thrust: CurrentThrust | undefined) => Promise<boolean>;

export class Engine extends BaseModule<lowlevel.Engine> {
    readonly type = ModuleType.ENGINE;

    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Engine(session)]);
    }

    async get_specification()
        : Promise<[Status, EngineSpecification | undefined]>
    {
        return await this.run(async (session) => this._get_specification(session));
    }

    async get_thrust()
        : Promise<[Status, CurrentThrust | undefined]>
    {
        return await this.run(async (session) => this._get_thrust(session));
    }

    async set_thrust(x: number, y: number, thrust: number,
                     duration_ms: number = 0, at?: number): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => session.send_change_thrust(x, y, thrust, duration_ms, at));
    }

    async monitoring(
        callback: MonitoringCallback,
        heartbeat_ms: number = 200): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._monitoring(session, callback, heartbeat_ms),
            true);
    }

    private async _get_specification(session: lowlevel.Engine)
        : Promise<[Status, EngineSpecification | undefined]>
    {
        const send_status = await session.send_specification_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, spec] = await session.wait_specification();
        if (!status.is_ok() || !spec) {
            return [status.wrap("failed to get engine specification"), undefined];
        }
        return [Status.ok(), spec];
    }

    private async _get_thrust(session: lowlevel.Engine)
        : Promise<[Status, CurrentThrust | undefined]>
    {
        const send_status = await session.send_thrust_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, thrust] = await session.wait_thrust();
        if (!status.is_ok() || !thrust) {
            return [status.wrap("failed to get engine thrust"), undefined];
        }
        return [Status.ok(), thrust];
    }

    private async _monitoring(
        session: lowlevel.Engine,
        callback: MonitoringCallback,
        heartbeat_ms: number): Promise<Status>
    {
        const send_status = await session.send_monitor_request();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send monitor request");
        }

        const [start_status, start_thrust] = await session.wait_thrust(2000);
        if (!start_status.is_ok() || !start_thrust) {
            return start_status.wrap("failed to start monitoring");
        }
        if (!await callback(start_thrust)) {
            return Status.ok();
        }

        while (true) {
            const [status, thrust] = await session.wait_thrust(heartbeat_ms);
            if (status.is_timeout()) {
                const resume = await callback(undefined);
                if (!resume) {
                    return Status.ok();
                }
                continue;
            }
            if (!status.is_ok() || !thrust) {
                return status.wrap("monitoring stopped");
            }
            const resume = await callback(thrust);
            if (!resume) {
                return Status.ok();
            }
        }
    }
}

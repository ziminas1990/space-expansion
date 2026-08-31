import * as lowlevel from "../lowlevel/index.js";
import { ServerTimestamp } from "../types/index.js";
import { Status } from "../types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type { ServerTimestamp };
export type MonitoringCallback =
    (timestamp: ServerTimestamp | undefined) => Promise<boolean>;

export class SystemClock extends BaseModule<lowlevel.SystemClock> {
    readonly type = ModuleType.SYSTEM_CLOCK;
    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.SystemClock(session)]);
    }

    async get_time()
        : Promise<[Status, ServerTimestamp | undefined]>
    {
        return await this.run(async (session) => this._get_time(session));
    }

    async wait_until(time_us: bigint, timeout_ms: number)
        : Promise<[Status, ServerTimestamp | undefined]>
    {
        return await this.run(
            async (session) => this._wait_until(session, time_us, timeout_ms));
    }

    async wait_for(period_us: bigint, timeout_ms: number)
        : Promise<[Status, ServerTimestamp | undefined]>
    {
        return await this.run(
            async (session) => this._wait_for(session, period_us, timeout_ms));
    }

    async monitoring(interval_ms: number, callback: MonitoringCallback)
        : Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._monitoring(session, interval_ms, callback),
            true);
    }

    private async _get_time(session: lowlevel.SystemClock)
        : Promise<[Status, ServerTimestamp | undefined]>
    {
        const send_status = await session.send_time_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, timestamp] = await session.wait_time();
        if (!status.is_ok() || !timestamp) {
            return [status.wrap("failed to get time"), undefined];
        }
        return [Status.ok(), timestamp];
    }

    private async _wait_until(
        session: lowlevel.SystemClock,
        time_us: bigint,
        timeout_ms: number)
        : Promise<[Status, ServerTimestamp | undefined]>
    {
        const send_status = await session.send_wait_until(time_us);
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to send wait until request"), undefined];
        }
        const [status, timestamp] = await session.wait_ring(timeout_ms);
        if (!status.is_ok() || !timestamp) {
            return [status.wrap("failed to wait until"), undefined];
        }
        return [Status.ok(), timestamp];
    }

    private async _wait_for(
        session: lowlevel.SystemClock,
        period_us: bigint,
        timeout_ms: number)
        : Promise<[Status, ServerTimestamp | undefined]>
    {
        const send_status = await session.send_wait_for(period_us);
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to send wait for request"), undefined];
        }
        const [status, timestamp] = await session.wait_ring(timeout_ms);
        if (!status.is_ok() || !timestamp) {
            return [status.wrap("failed to wait for"), undefined];
        }
        return [Status.ok(), timestamp];
    }

    private async _monitoring(
        session: lowlevel.SystemClock,
        interval_ms: number,
        callback: MonitoringCallback): Promise<Status>
    {
        const send_status = await session.send_monitor_request(interval_ms);
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send monitor request");
        }

        const timeout_ms = Math.max(500, interval_ms * 10);
        while (true) {
            const [status, timestamp] = await session.wait_time(timeout_ms);
            if (status.is_timeout()) {
                // Heartbeat so the caller can stop by returning false.
                const resume = await callback(undefined);
                if (!resume) {
                    return Status.ok();
                }
                continue;
            }
            if (!status.is_ok() || !timestamp) {
                return status.wrap("monitoring stopped");
            }
            const resume = await callback(timestamp);
            if (!resume) {
                return Status.ok();
            }
        }
    }
}

import * as lowlevel from "#sdk/lowlevel/index.js";
import { ResourceItem } from "#sdk/types/index.js";
import { Status } from "#sdk/types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type AsteroidMinerStatus = lowlevel.AsteroidMinerStatus;
export type AsteroidMinerSpecification = lowlevel.AsteroidMinerSpecification;
export type MiningCallback =
    (resources: ResourceItem[]) => Promise<boolean>;

export class AsteroidMiner extends BaseModule<lowlevel.AsteroidMiner> {
    readonly type = ModuleType.ASTEROID_MINER;
    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.AsteroidMiner(session)]);
    }

    async get_specification()
        : Promise<[Status, AsteroidMinerSpecification | undefined]>
    {
        return await this.run(async (session) => this._get_specification(session));
    }

    async bind_to_cargo(cargo_name: string): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._bind_to_cargo(session, cargo_name));
    }

    async stop_mining(): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._stop_mining(session));
    }

    async start_mining(
        asteroid_id: number,
        progress_cb: MiningCallback,
        timeout_ms?: number): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._start_mining(
                session, asteroid_id, progress_cb, timeout_ms),
            true);
    }

    private async _get_specification(session: lowlevel.AsteroidMiner)
        : Promise<[Status, AsteroidMinerSpecification | undefined]>
    {
        const send_status = await session.send_specification_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, spec] = await session.wait_specification();
        if (!status.is_ok() || !spec) {
            return [status.wrap("failed to get asteroid miner specification"),
                    undefined];
        }
        return [Status.ok(), spec];
    }

    private async _bind_to_cargo(
        session: lowlevel.AsteroidMiner,
        cargo_name: string): Promise<Status>
    {
        const send_status = await session.send_bind_to_cargo(cargo_name);
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send bind to cargo request");
        }
        const [status, server_status] = await session.wait_bind_to_cargo_status();
        if (!status.is_ok() || !server_status) {
            return status.wrap("failed to bind to cargo");
        }
        if (server_status !== "SUCCESS") {
            return Status.fail(server_status);
        }
        return Status.ok();
    }

    private async _stop_mining(session: lowlevel.AsteroidMiner): Promise<Status>
    {
        const send_status = await session.send_stop_mining();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send stop mining request");
        }
        const [status, server_status] = await session.wait_stop_mining_status();
        if (!status.is_ok() || !server_status) {
            return status.wrap("failed to stop mining");
        }
        if (server_status !== "SUCCESS") {
            return Status.fail(server_status);
        }
        return Status.ok();
    }

    private async _start_mining(
        session: lowlevel.AsteroidMiner,
        asteroid_id: number,
        progress_cb: MiningCallback,
        timeout_ms?: number): Promise<Status>
    {
        let report_timeout = timeout_ms;
        if (report_timeout === undefined) {
            const [spec_status, spec] = await this._get_specification(session);
            if (!spec_status.is_ok() || !spec) {
                return spec_status.wrap("failed to get asteroid miner specification");
            }
            report_timeout = Math.max(500, Math.ceil(spec.cycle_time_ms * 2.1));
        }

        const send_status = await session.send_start_mining(asteroid_id);
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send start mining request");
        }

        const [start_status, mining_status] = await session.wait_start_mining_status();
        if (!start_status.is_ok() || !mining_status) {
            return start_status.wrap("failed to start mining");
        }
        if (mining_status !== "SUCCESS") {
            return Status.fail(mining_status);
        }

        while (true) {
            const [status, event] = await session.wait_mining_event(report_timeout);
            if (!status.is_ok() || !event) {
                return status.wrap("failed to get mining report");
            }
            if (event.case === "mining_is_stopped") {
                return Status.fail(event.status);
            }

            const resume = await progress_cb(event.resources);
            if (!resume) {
                const stop_status = await this._stop_mining(session);
                if (!stop_status.is_ok()) {
                    return stop_status;
                }
                return Status.ok();
            }
        }
    }
}

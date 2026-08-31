import * as lowlevel from "../lowlevel/index.js";
import { PhysicalObject } from "../types/index.js";
import { Status } from "../types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type PassiveScannerSpecification = lowlevel.PassiveScannerSpecification;
export type MonitoringCallback =
    (objects: PhysicalObject[] | undefined) => Promise<boolean>;

export class PassiveScanner extends BaseModule<lowlevel.PassiveScanner> {
    readonly type = ModuleType.PASSIVE_SCANNER;
    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.PassiveScanner(session)]);
    }

    async get_specification()
        : Promise<[Status, PassiveScannerSpecification | undefined]>
    {
        return await this.run(async (session) => this._get_specification(session));
    }

    async monitoring(callback: MonitoringCallback): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._monitoring(session, callback), true);
    }

    private async _get_specification(session: lowlevel.PassiveScanner)
        : Promise<[Status, PassiveScannerSpecification | undefined]>
    {
        const send_status = await session.send_specification_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, spec] = await session.wait_specification();
        if (!status.is_ok() || !spec) {
            return [status.wrap("failed to get passive scanner specification"), undefined];
        }
        return [Status.ok(), spec];
    }

    private async _monitoring(
        session: lowlevel.PassiveScanner,
        callback: MonitoringCallback): Promise<Status>
    {
        const send_status = await session.send_monitor_request();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send monitor request");
        }

        const [ack_status, ack] = await session.wait_monitor_ack();
        if (!ack_status.is_ok() || ack === undefined) {
            return ack_status.wrap("failed to start monitoring");
        }
        if (!ack) {
            return Status.fail("MONITORING_FAILED");
        }

        while (true) {
            const [status, objects] = await session.wait_update(200);
            if (status.is_timeout()) {
                const resume = await callback(undefined);
                if (!resume) {
                    return Status.ok();
                }
                continue;
            }
            if (!status.is_ok()) {
                return status.wrap("monitoring stopped");
            }
            const resume = await callback(objects);
            if (!resume) {
                return Status.ok();
            }
        }
    }
}

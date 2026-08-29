import * as lowlevel from "../lowlevel/index.js";
import { Status } from "../types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type AsteroidScannerStatus = lowlevel.AsteroidScannerStatus;
export type AsteroidScannerSpecification = lowlevel.AsteroidScannerSpecification;
export type AsteroidScanResult = lowlevel.AsteroidScanResult;

export class AsteroidScanner extends BaseModule<lowlevel.AsteroidScanner> {
    readonly type = ModuleType.ASTEROID_SCANNER;
    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.AsteroidScanner(session)]);
    }

    async get_specification()
        : Promise<[Status, AsteroidScannerSpecification | undefined]>
    {
        return await this.run(async (session) => this._get_specification(session));
    }

    async scan(asteroid_id: number, timeout_ms?: number)
        : Promise<[Status, AsteroidScanResult | undefined]>
    {
        return await this.run(
            async (session) => this._scan(session, asteroid_id, timeout_ms), true);
    }

    private async _get_specification(session: lowlevel.AsteroidScanner)
        : Promise<[Status, AsteroidScannerSpecification | undefined]>
    {
        const send_status = await session.send_specification_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, spec] = await session.wait_specification();
        if (!status.is_ok() || !spec) {
            return [status.wrap("failed to get asteroid scanner specification"), undefined];
        }
        return [Status.ok(), spec];
    }

    private async _scan(
        session: lowlevel.AsteroidScanner,
        asteroid_id: number,
        timeout_ms?: number)
        : Promise<[Status, AsteroidScanResult | undefined]>
    {
        let finish_timeout = timeout_ms;
        if (finish_timeout === undefined) {
            const [spec_status, spec] = await this._get_specification(session);
            if (!spec_status.is_ok() || !spec) {
                return [spec_status.wrap("failed to get asteroid scanner specification"),
                        undefined];
            }
            finish_timeout = Math.max(500, spec.scanning_time_ms * 2);
        }

        const send_status = await session.send_scan_asteroid(asteroid_id);
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to send scan request"), undefined];
        }

        const [status, scanning_status] = await session.wait_scanning_status();
        if (!status.is_ok() || !scanning_status) {
            return [status.wrap("failed to get scanning status"), undefined];
        }
        if (scanning_status !== "IN_PROGRESS") {
            return [Status.fail(scanning_status), undefined];
        }

        const [finish_status, result] = await session.wait_scanning_finished(finish_timeout);
        if (!finish_status.is_ok() || !result) {
            return [finish_status.wrap("scanning did not finish"), undefined];
        }
        return [Status.ok(), result];
    }
}

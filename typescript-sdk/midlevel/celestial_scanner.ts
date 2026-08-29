import * as lowlevel from "../lowlevel/index.js";
import { PhysicalObject } from "../types/index.js";
import { Status } from "../types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type CelestialScannerStatus = lowlevel.CelestialScannerStatus;
export type CelestialScannerSpecification = lowlevel.CelestialScannerSpecification;
export type ScanningCallback =
    (asteroids: PhysicalObject[]) => Promise<boolean>;

export class CelestialScanner extends BaseModule<lowlevel.CelestialScanner> {
    readonly type = ModuleType.CELESTIAL_SCANNER;
    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.CelestialScanner(session)]);
    }

    static expected_scanning_time(
        spec: CelestialScannerSpecification,
        scanning_radius_km: number,
        minimal_radius_m: number): number
    {
        const resolution = (1000 * scanning_radius_km) / minimal_radius_m;
        const c_km_per_sec = 300000;
        const total_processing_time_s = (resolution * spec.processing_time_us) / 1_000_000;
        return 0.1 + 2 * scanning_radius_km / c_km_per_sec + total_processing_time_s;
    }

    async get_specification()
        : Promise<[Status, CelestialScannerSpecification | undefined]>
    {
        return await this.run(async (session) => this._get_specification(session));
    }

    async scan(
        scanning_radius_km: number,
        minimal_radius_m: number,
        callback: ScanningCallback,
        timeout_ms?: number)
        : Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._scan(
                session, scanning_radius_km, minimal_radius_m, callback, timeout_ms),
            true);
    }

    async scan_sync(
        scanning_radius_km: number,
        minimal_radius_m: number,
        timeout_ms?: number)
        : Promise<[Status, PhysicalObject[] | undefined]>
    {
        return await this.run(
            async (session) => {
                const asteroids: PhysicalObject[] = [];
                const status = await this._scan(
                    session,
                    scanning_radius_km,
                    minimal_radius_m,
                    async (page) => {
                        asteroids.push(...page);
                        return true;
                    },
                    timeout_ms);
                if (!status.is_ok()) {
                    return [status, undefined];
                }
                return [Status.ok(), asteroids];
            },
            true);
    }

    private async _get_specification(session: lowlevel.CelestialScanner)
        : Promise<[Status, CelestialScannerSpecification | undefined]>
    {
        const send_status = await session.send_specification_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, spec] = await session.wait_specification();
        if (!status.is_ok() || !spec) {
            return [status.wrap("failed to get celestial scanner specification"), undefined];
        }
        return [Status.ok(), spec];
    }

    private async _scan(
        session: lowlevel.CelestialScanner,
        scanning_radius_km: number,
        minimal_radius_m: number,
        callback: ScanningCallback,
        timeout_ms?: number): Promise<Status>
    {
        let finish_timeout = timeout_ms;
        if (finish_timeout === undefined) {
            const [spec_status, spec] = await this._get_specification(session);
            if (!spec_status.is_ok() || !spec) {
                return spec_status.wrap("can't calculate expected timeout");
            }
            const expected_s = CelestialScanner.expected_scanning_time(
                spec, scanning_radius_km, minimal_radius_m);
            finish_timeout = Math.max(200, 2 * expected_s * 1000);
        }

        const send_status = await session.send_scan_request(
            scanning_radius_km, minimal_radius_m);
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send scan request");
        }

        while (true) {
            const [status, result] = await session.wait_scanning_report(finish_timeout);
            if (!status.is_ok() || !result) {
                return status.wrap("failed to get scanning report");
            }
            if (result.case === "scanning_failed") {
                return Status.fail(result.status);
            }

            const resume = await callback(result.report.asteroids);
            if (!resume) {
                return Status.ok();
            }
            if (result.report.left === 0) {
                return Status.ok();
            }
        }
    }
}

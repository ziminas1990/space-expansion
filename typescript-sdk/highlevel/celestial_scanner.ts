import * as midlevel from "../midlevel/index.js";
import { PhysicalObject, Status } from "../types/index.js";
import { Cached } from "../utils/cache.js";
import { EventEmitter } from "./events.js";
import type { BaseModule } from "./base_module.js";

export type CelestialScannerSpecification = midlevel.CelestialScannerSpecification;
export type CelestialScannerStatus = midlevel.CelestialScannerStatus;

export type Events = {
    asteroids: (asteroids: PhysicalObject[]) => Promise<void> | void;
}

// NOTE: The server does not stream objects as they are discovered. It completes
// the scan first and then sends all result pages, so the high-level API returns
// the complete result instead of exposing a callback or stream.
export class CelestialScanner extends EventEmitter<Events> implements BaseModule {
    readonly type = midlevel.ModuleType.CELESTIAL_SCANNER;
    private specification = new Cached<CelestialScannerSpecification>();
    private last_asteroids: PhysicalObject[] = [];

    constructor(
        private rpc: midlevel.CelestialScanner,
        readonly name: string,
    ) {
        super();
    }

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.CELESTIAL_SCANNER)) {
            return Status.fail("expected CelestialScanner");
        }
        await this.release();
        this.rpc = rpc;
        return Status.ok();
    }

    down_level(): midlevel.CelestialScanner {
        return this.rpc;
    }

    asteroids(): PhysicalObject[] {
        return this.last_asteroids;
    }

    async get_specification(
        reset_cached: boolean = false,
    ): Promise<[Status, CelestialScannerSpecification | undefined]> {
        if (reset_cached) {
            this.specification.reset();
        } else {
            const cached = this.specification.get(Infinity);
            if (cached) {
                return [Status.ok(), cached];
            }
        }
        const [status, spec] = await this.rpc.get_specification();
        if (!status.is_ok() || !spec) {
            return [status, undefined];
        }
        this.specification.set(spec);
        return [Status.ok(), spec];
    }

    expected_scanning_time(
        scanning_radius_km: number,
        minimal_radius_m: number,
    ): number | undefined {
        const spec = this.specification.get(Infinity);
        if (!spec) {
            return undefined;
        }
        return midlevel.CelestialScanner.expected_scanning_time(
            spec, scanning_radius_km, minimal_radius_m);
    }

    async scan(
        scanning_radius_km: number,
        minimal_radius_m: number,
        timeout_ms?: number,
    ): Promise<[Status, PhysicalObject[] | undefined]> {
        const [timeout_status, finish_timeout] = await this.scan_timeout(
            scanning_radius_km, minimal_radius_m, timeout_ms);
        if (finish_timeout === undefined) {
            return [timeout_status, undefined];
        }
        const [status, asteroids] = await this.rpc.scan_sync(
            scanning_radius_km, minimal_radius_m, finish_timeout);
        if (status.is_ok() && asteroids) {
            await this.store_asteroids(asteroids);
        }
        return [status, asteroids];
    }

    async release(): Promise<Status> {
        this.specification.reset();
        this.last_asteroids = [];
        return Status.ok();
    }

    private async scan_timeout(
        scanning_radius_km: number,
        minimal_radius_m: number,
        timeout_ms?: number,
    ): Promise<[Status, number | undefined]> {
        if (timeout_ms !== undefined) {
            return [Status.ok(), timeout_ms];
        }
        let expected_s = this.expected_scanning_time(
            scanning_radius_km, minimal_radius_m);
        if (expected_s === undefined) {
            const [status, spec] = await this.get_specification();
            if (!status.is_ok() || !spec) {
                return [status.wrap("can't calculate expected timeout"), undefined];
            }
            expected_s = midlevel.CelestialScanner.expected_scanning_time(
                spec, scanning_radius_km, minimal_radius_m);
        }
        return [Status.ok(), Math.max(200, 2 * expected_s * 1000)];
    }

    private async store_asteroids(asteroids: PhysicalObject[]): Promise<void> {
        this.last_asteroids = asteroids;
        await this.emit("asteroids", asteroids);
    }

}

import * as midlevel from "#sdk/midlevel/index.js";
import { Status } from "#sdk/types/index.js";
import { Cached } from "#sdk/utils/cache.js";
import type { BaseModule } from "./base_module.js";

export type AsteroidScannerSpecification = midlevel.AsteroidScannerSpecification;
export type AsteroidScanResult = midlevel.AsteroidScanResult;
export type AsteroidScannerStatus = midlevel.AsteroidScannerStatus;

export class AsteroidScanner implements BaseModule {
    readonly type = midlevel.ModuleType.ASTEROID_SCANNER;

    private specification = new Cached<AsteroidScannerSpecification>();
    private results = new Map<number, AsteroidScanResult>();
    private busy = false;

    constructor(
        private rpc: midlevel.AsteroidScanner,
        readonly name: string,
    ) {}

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.ASTEROID_SCANNER)) {
            return Status.fail("expected AsteroidScanner");
        }
        await this.release();
        this.rpc = rpc;
        return Status.ok();
    }

    down_level(): midlevel.AsteroidScanner {
        return this.rpc;
    }

    async get_specification(
        reset_cached: boolean = false,
    ): Promise<[Status, AsteroidScannerSpecification | undefined]> {
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

    scan_result(asteroid_id: number): AsteroidScanResult | undefined {
        return this.results.get(asteroid_id);
    }

    async scan(
        asteroid_id: number,
        timeout_ms?: number,
    ): Promise<[Status, AsteroidScanResult | undefined]> {
        if (this.busy) {
            return [Status.fail("SCANNER_BUSY"), undefined];
        }
        this.busy = true;
        try {
            let finish_timeout = timeout_ms;
            if (finish_timeout === undefined) {
                const [spec_status, spec] = await this.get_specification();
                if (!spec_status.is_ok() || !spec) {
                    return [
                        spec_status.wrap("failed to get asteroid scanner specification"),
                        undefined,
                    ];
                }
                finish_timeout = Math.max(500, spec.scanning_time_ms * 2);
            }
            const [status, result] = await this.rpc.scan(asteroid_id, finish_timeout);
            if (status.is_ok() && result) {
                this.results.set(asteroid_id, result);
            }
            return [status, result];
        } finally {
            this.busy = false;
        }
    }

    async release(): Promise<Status> {
        await this.rpc.terminate();
        this.specification.reset();
        this.results.clear();
        return Status.ok();
    }

}

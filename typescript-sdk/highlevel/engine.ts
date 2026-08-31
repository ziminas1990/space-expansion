import * as midlevel from "../midlevel/index.js";
import { Status } from "../types/index.js";
import { Cached } from "../utils/cache.js";
import type { BaseModule } from "./base_module.js";

export type EngineSpecification = midlevel.EngineSpecification;
export type CurrentThrust = midlevel.CurrentThrust;

const DEFAULT_THRUST_CACHE_MS = 100;

export class Engine implements BaseModule {
    readonly type = midlevel.ModuleType.ENGINE;
    private specification = new Cached<EngineSpecification>();
    private thrust = new Cached<CurrentThrust>();

    constructor(
        private rpc: midlevel.Engine,
        readonly name: string,
    ) {}

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.ENGINE)) {
            return Status.fail("expected Engine");
        }
        await this.release();
        this.rpc = rpc;
        return Status.ok();
    }

    down_level(): midlevel.Engine {
        return this.rpc;
    }

    async get_specification(
        reset_cached: boolean = false,
    ): Promise<[Status, EngineSpecification | undefined]> {
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

    async get_thrust(
        cache_expiring_ms: number = DEFAULT_THRUST_CACHE_MS,
    ): Promise<[Status, CurrentThrust | undefined]> {
        const cached = this.thrust.get(cache_expiring_ms);
        if (cached) {
            return [Status.ok(), cached];
        }
        const [status, thrust] = await this.rpc.get_thrust();
        if (!status.is_ok() || !thrust) {
            return [status, undefined];
        }
        this.thrust.set(thrust);
        return [Status.ok(), thrust];
    }

    async set_thrust(
        x: number,
        y: number,
        thrust: number,
        duration_ms: number = 0,
        at?: bigint,
    ): Promise<Status> {
        const status = await this.rpc.set_thrust(x, y, thrust, duration_ms, at);
        this.thrust.reset();
        return status;
    }

    async release(): Promise<Status> {
        await this.rpc.terminate();
        this.specification.reset();
        this.thrust.reset();
        return Status.ok();
    }

}

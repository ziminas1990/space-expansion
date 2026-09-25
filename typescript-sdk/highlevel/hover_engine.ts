import * as midlevel from "#sdk/midlevel/index.js";
import { Status } from "#sdk/types/index.js";
import { Cached } from "#sdk/utils/cache.js";
import type { BaseModule } from "./base_module.js";

export type HoverEngineSpecification = midlevel.HoverEngineSpecification;

export class HoverEngine implements BaseModule {
    readonly type = midlevel.ModuleType.HOVER_ENGINE;
    private specification = new Cached<HoverEngineSpecification>();

    constructor(
        private rpc: midlevel.HoverEngine,
        readonly name: string,
    ) {}

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.HOVER_ENGINE)) {
            return Status.fail("expected HoverEngine");
        }
        await this.release();
        this.rpc = rpc;
        return Status.ok();
    }

    down_level(): midlevel.HoverEngine {
        return this.rpc;
    }

    async get_specification(
        reset_cached: boolean = false,
    ): Promise<[Status, HoverEngineSpecification | undefined]> {
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

    async get_thrust(): Promise<[Status, number | undefined]> {
        return await this.rpc.get_thrust();
    }

    async set_thrust(
        thrust: number,
        duration_ms: number = 0,
        at?: number,
    ): Promise<Status> {
        return await this.rpc.set_thrust(thrust, duration_ms, at);
    }

    async release(): Promise<Status> {
        await this.rpc.terminate();
        this.specification.reset();
        return Status.ok();
    }
}

import * as midlevel from "#sdk/midlevel/index.js";
import { Status } from "#sdk/types/index.js";
import { Cached } from "#sdk/utils/cache.js";
import { EventEmitter } from "./events.js";
import type { BaseModule } from "./base_module.js";

export type RCSSpecification = midlevel.RCSSpecification;
export type CurrentThrust = midlevel.CurrentThrust;

export type Events = {
    thrust: (thrust: CurrentThrust) => Promise<void> | void;
    // Emitted when RCS goes offline and stops monitoring
    offline: (status: Status) => Promise<void> | void;
}

const DEFAULT_THRUST_CACHE_MS = 100;

export class RCS extends EventEmitter<Events> implements BaseModule {
    readonly type = midlevel.ModuleType.RCS;
    private specification = new Cached<RCSSpecification>();
    private thrust = new Cached<CurrentThrust>();
    private stopped = false;
    private loop?: Promise<void>;
    private in_callback = false;

    constructor(
        private rpc: midlevel.RCS,
        readonly name: string,
    ) {
        super();
    }

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.RCS)) {
            return Status.fail("expected RCS");
        }
        await this.release();
        this.rpc = rpc;
        return await this.init();
    }

    down_level(): midlevel.RCS {
        return this.rpc;
    }

    async init(): Promise<Status> {
        this.stopped = false;
        this.loop ??= this.monitor_loop();
        return Status.ok();
    }

    async get_specification(
        reset_cached: boolean = false,
    ): Promise<[Status, RCSSpecification | undefined]> {
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
        at?: number,
    ): Promise<Status> {
        const status = await this.rpc.set_thrust(x, y, thrust, duration_ms, at);
        this.thrust.reset();
        return status;
    }

    async release(): Promise<Status> {
        this.stopped = true;
        await this.rpc.terminate();
        if (this.loop && !this.in_callback) {
            await this.loop;
            this.loop = undefined;
        }
        this.specification.reset();
        this.thrust.reset();
        return Status.ok();
    }

    private async monitor_loop(): Promise<void> {
        while (!this.stopped) {
            try {
                const status = await this.rpc.monitoring(async (thrust) => {
                    if (thrust) {
                        await this.apply_thrust(thrust);
                    }
                    return !this.stopped;
                }, 100);
                if (!status.is_ok()) {
                    await this.notify_offline(status);
                    return;
                }
            } catch (error) {
                await this.notify_offline(Status.exception(error));
                return;
            }
        }
    }

    private async notify_offline(status: Status): Promise<void> {
        this.loop = undefined;
        this.in_callback = true;
        try {
            await this.emit("offline", status);
        } finally {
            this.in_callback = false;
        }
    }

    private async apply_thrust(thrust: CurrentThrust): Promise<void> {
        this.thrust.set(thrust);
        this.in_callback = true;
        try {
            await this.emit("thrust", thrust);
        } finally {
            this.in_callback = false;
        }
    }

}

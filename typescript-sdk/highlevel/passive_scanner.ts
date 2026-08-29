import * as midlevel from "../midlevel/index.js";
import { PhysicalObject, Status } from "../types/index.js";
import { Cached } from "../utils/cache.js";
import { EventEmitter } from "./events.js";
import type { BaseModule } from "./base_module.js";
import { extrapolate } from "./navigation.js";

export type PassiveScannerSpecification = midlevel.PassiveScannerSpecification;

export type Events = {
    update: (object: PhysicalObject) => Promise<void> | void;
    lost: (object: PhysicalObject) => Promise<void> | void;
    // Emitted when the scanner goes offline and stops monitoring
    offline: (status: Status) => Promise<void> | void;
}

type TrackedObject = {
    object: PhysicalObject;
    last_seen_us: bigint;
};

export class PassiveScanner extends EventEmitter<Events> implements BaseModule {
    readonly type = midlevel.ModuleType.PASSIVE_SCANNER;
    private specification = new Cached<PassiveScannerSpecification>();
    private detected = new Map<number, TrackedObject>();
    private stopped = false;
    private loop?: Promise<void>;

    constructor(
        private rpc: midlevel.PassiveScanner,
        readonly name: string,
    ) {
        super();
    }

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.PASSIVE_SCANNER)) {
            return Status.fail("expected PassiveScanner");
        }
        await this.release();
        this.rpc = rpc;
        return Status.ok();
    }

    down_level(): midlevel.PassiveScanner {
        return this.rpc;
    }

    start_monitoring(): Status {
        this.stopped = false;
        this.loop ??= this.monitor_loop();
        return Status.ok();
    }

    async get_specification(
        reset_cached: boolean = false,
    ): Promise<[Status, PassiveScannerSpecification | undefined]> {
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

    objects(): PhysicalObject[] {
        return [...this.detected.values()].map((tracked) => tracked.object);
    }

    predict_objects(at_us: bigint): PhysicalObject[] {
        return this.objects().map((object) => ({
            ...object,
            position: extrapolate(object.position, at_us),
        }));
    }

    async release(): Promise<Status> {
        this.stopped = true;
        if (this.loop) {
            await this.loop;
            this.loop = undefined;
        }
        this.specification.reset();
        this.detected.clear();
        return Status.ok();
    }

    private async monitor_loop(): Promise<void> {
        while (!this.stopped) {
            await this.get_specification();
            try {
                const status = await this.rpc.monitoring(async (objects) => {
                    if (objects.length > 0) {
                        await this.apply_objects(objects);
                        const at_us = objects.reduce(
                            (latest, object) => object.position.timestamp > latest
                                ? object.position.timestamp
                                : latest,
                            0n);
                        await this.drop_lost(at_us);
                    }
                    return !this.stopped;
                });
                if (!status.is_ok()) {
                    this.loop = undefined;
                    await this.emit("offline", status);
                    return;
                }
            } catch (error) {
                this.loop = undefined;
                await this.emit("offline", Status.exception(error));
                return;
            }
        }
    }

    private async apply_objects(objects: PhysicalObject[]): Promise<void> {
        for (const object of objects) {
            const existing = this.detected.get(object.object_id);
            if (existing && object.position.timestamp <= existing.last_seen_us) {
                continue;
            }
            this.detected.set(object.object_id, {
                object,
                last_seen_us: object.position.timestamp,
            });
            await this.emit("update", object);
        }
    }

    private async drop_lost(at_us: bigint): Promise<void> {
        const spec = this.specification.get(Infinity);
        if (!spec) {
            return;
        }
        const threshold_us = BigInt(spec.max_update_time_ms) * 2_000n;
        const lost: PhysicalObject[] = [];
        for (const [object_id, tracked] of this.detected) {
            if (at_us > tracked.last_seen_us + threshold_us) {
                this.detected.delete(object_id);
                lost.push(tracked.object);
            }
        }
        for (const object of lost) {
            await this.emit("lost", object);
        }
    }

}

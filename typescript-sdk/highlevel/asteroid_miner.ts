import * as midlevel from "../midlevel/index.js";
import { ResourceItem, ResourceType, Status } from "../types/index.js";
import { Cached } from "../utils/cache.js";
import { EventEmitter } from "./events.js";
import type { BaseModule } from "./base_module.js";

export type AsteroidMinerSpecification = midlevel.AsteroidMinerSpecification;
export type AsteroidMinerStatus = midlevel.AsteroidMinerStatus;

export type Events = {
    mining_report: (resources: ResourceItem[]) => Promise<void> | void;
    mining_stopped: (status: Status) => Promise<void> | void;
}

export class AsteroidMiner extends EventEmitter<Events> implements BaseModule {
    readonly type = midlevel.ModuleType.ASTEROID_MINER;
    private specification = new Cached<AsteroidMinerSpecification>();
    private mined = new Map<ResourceType, number>();
    private mining = false;
    private stop_requested = false;
    private in_callback = false;
    private mining_task?: Promise<Status>;
    cargo_name: string | undefined = undefined;

    constructor(
        private rpc: midlevel.AsteroidMiner,
        readonly name: string,
    ) {
        super();
    }

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.ASTEROID_MINER)) {
            return Status.fail("expected AsteroidMiner");
        }
        await this.release();
        this.rpc = rpc;
        return Status.ok();
    }

    down_level(): midlevel.AsteroidMiner {
        return this.rpc;
    }

    is_mining(): boolean {
        return this.mining;
    }

    mined_resources(): ResourceItem[] {
        return [...this.mined.entries()].map(([resource_type, amount]) => ({
            resource_type,
            amount,
        }));
    }

    async get_specification(
        reset_cached: boolean = false,
    ): Promise<[Status, AsteroidMinerSpecification | undefined]> {
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

    async bind_to_cargo(cargo_name: string): Promise<Status> {
        const status = await this.rpc.bind_to_cargo(cargo_name);
        if (status.is_ok()) {
            this.cargo_name = cargo_name;
        }
        return status;
    }

    async start_mining(
        asteroid_id: number,
        timeout_ms?: number,
    ): Promise<Status> {
        if (this.mining) {
            return Status.fail("MINER_IS_BUSY");
        }
        this.mining = true;
        this.stop_requested = false;
        this.mined.clear();
        try {
            let report_timeout = timeout_ms;
            if (report_timeout === undefined) {
                const [spec_status, spec] = await this.get_specification();
                if (!spec_status.is_ok() || !spec) {
                    return spec_status.wrap(
                        "failed to get asteroid miner specification");
                }
                report_timeout = Math.max(500, Math.ceil(spec.cycle_time_ms * 3));
            }
            const task = this.run_mining(asteroid_id, report_timeout);
            this.mining_task = task;
            try {
                return await task;
            } finally {
                if (this.mining_task === task) {
                    this.mining_task = undefined;
                }
            }
        } finally {
            this.mining = false;
        }
    }

    async stop_mining(): Promise<Status> {
        this.stop_requested = true;
        const status = await this.rpc.stop_mining();
        if (this.mining_task && !this.in_callback) {
            await this.mining_task;
        }
        return status;
    }

    async release(): Promise<Status> {
        this.stop_requested = true;
        if (this.mining) {
            await this.rpc.stop_mining();
            if (this.mining_task && !this.in_callback) {
                await this.mining_task;
            }
        }
        this.specification.reset();
        this.mined.clear();
        this.cargo_name = undefined;
        this.mining = false;
        return Status.ok();
    }

    private async run_mining(
        asteroid_id: number,
        timeout_ms: number,
    ): Promise<Status> {
        const status = await this.rpc.start_mining(
            asteroid_id,
            async (resources) => {
                this.accumulate(resources);
                this.in_callback = true;
                try {
                    await this.emit("mining_report", resources);
                } finally {
                    this.in_callback = false;
                }
                return !this.stop_requested;
            },
            timeout_ms,
        );
        await this.emit("mining_stopped", status);
        return status;
    }

    private accumulate(resources: ResourceItem[]): void {
        for (const item of resources) {
            this.mined.set(
                item.resource_type,
                (this.mined.get(item.resource_type) ?? 0) + item.amount,
            );
        }
    }

}

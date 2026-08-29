import * as midlevel from "../midlevel/index.js";
import { Status } from "../types/index.js";
import { Cached } from "../utils/cache.js";
import { EventEmitter } from "./events.js";
import type { BaseModule } from "./base_module.js";

export type ShipyardSpecification = midlevel.ShipyardSpecification;
export type ShipyardStatus = midlevel.ShipyardStatus;
export type ShipyardShipBuilt = midlevel.ShipyardShipBuilt;

export type BuildingCallback =
    (status: ShipyardStatus, progress: number) => Promise<void> | void;

export type Events = {
    build_progress: (status: ShipyardStatus, progress: number) => Promise<void> | void;
    build_complete: (ship: ShipyardShipBuilt) => Promise<void> | void;
    build_failed: (status: Status) => Promise<void> | void;
}

export class Shipyard extends EventEmitter<Events> implements BaseModule {
    readonly type = midlevel.ModuleType.SHIPYARD;
    private specification = new Cached<ShipyardSpecification>();
    private building = false;
    private build_task?: Promise<[Status, ShipyardShipBuilt | undefined]>;
    cargo_name: string | undefined = undefined;

    constructor(
        private rpc: midlevel.Shipyard,
        readonly name: string,
    ) {
        super();
    }


    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.SHIPYARD)) {
            return Status.fail("expected Shipyard");
        }
        await this.release();
        this.rpc = rpc;
        return Status.ok();
    }

    down_level(): midlevel.Shipyard {
        return this.rpc;
    }

    is_building(): boolean {
        return this.building;
    }

    async get_specification(
        reset_cached: boolean = false,
    ): Promise<[Status, ShipyardSpecification | undefined]> {
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

    async build_ship(
        blueprint: string,
        ship_name: string,
        progress_cb?: BuildingCallback,
    ): Promise<[Status, ShipyardShipBuilt | undefined]> {
        if (this.building) {
            return [Status.fail("SHIPYARD_IS_BUSY"), undefined];
        }
        this.building = true;
        const task = this.run_build(blueprint, ship_name, progress_cb);
        this.build_task = task;
        try {
            return await task;
        } finally {
            if (this.build_task === task) {
                this.build_task = undefined;
            }
            this.building = false;
        }
    }

    async cancel_build(): Promise<Status> {
        const status = await this.rpc.cancel_build();
        if (this.build_task) {
            await this.build_task;
        }
        this.building = false;
        return status;
    }

    async release(): Promise<Status> {
        this.specification.reset();
        this.cargo_name = undefined;
        this.building = false;
        return Status.ok();
    }

    private async run_build(
        blueprint: string,
        ship_name: string,
        progress_cb?: BuildingCallback,
    ): Promise<[Status, ShipyardShipBuilt | undefined]> {
        const [status, ship] = await this.rpc.build_ship(
            blueprint,
            ship_name,
            async (report_status, progress) => {
                await this.emit("build_progress", report_status, progress);
                if (progress_cb) {
                    await progress_cb(report_status, progress);
                }
            },
        );
        if (status.is_ok() && ship) {
            await this.emit("build_complete", ship);
        } else {
            await this.emit("build_failed", status);
        }
        return [status, ship];
    }

}

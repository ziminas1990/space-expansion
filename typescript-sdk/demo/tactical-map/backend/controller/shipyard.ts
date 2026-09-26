import * as midlevel from "@spx/sdk/midlevel";
import { ShipyardState } from "../../common/domain/shipyard.js";
import { IWorld } from "./interfaces.js";
import { ModuleMonitor } from "./module_monitor.js";
import { Logger } from "../log.js";

export class ShipyardController {
    private readonly monitor: ModuleMonitor<midlevel.ShipyardMonitoringEvent>;
    private state: ShipyardState = { status: "idle" };

    constructor(
        remote: midlevel.Shipyard,
        world: IWorld,
        logger: Logger,
        ship_id: string,
        slot_id: number,
        name: string,
    ) {
        this.monitor = new ModuleMonitor(remote, logger, name, (event) => {
            const state = this.apply_event(event);
            if (state === undefined) {
                return;
            }
            this.state = state;
            world.update({ type: "shipyard_update", ship_id, slot_id, state });
        });
    }

    start(): void {
        this.monitor.start();
    }

    async stop(): Promise<void> {
        await this.monitor.stop();
    }

    private apply_event(event: midlevel.ShipyardMonitoringEvent): ShipyardState | undefined {
        switch (event.case) {
            case "idle":
            case "building_complete":
                return { status: "idle" };
            case "build_started":
                return {
                    status: "building",
                    blueprint_name: event.build.blueprint_name,
                    ship_name: event.build.ship_name,
                    progress: 0,
                };
            case "building_report":
                if (event.report.status === "BUILD_CANCELED"
                    || event.report.status === "BUILD_FAILED") {
                    return { status: "idle" };
                }
                if (this.state.status === "idle") {
                    return undefined;
                }
                if (event.report.status === "BUILD_IN_PROGRESS"
                    || event.report.status === "BUILD_FROZEN"
                    || event.report.status === "BUILD_COMPLETE") {
                    return {
                        ...this.state,
                        status: event.report.status === "BUILD_FROZEN" ? "frozen" : "building",
                        progress: event.report.progress,
                    };
                }
                return undefined;
        }
    }
}

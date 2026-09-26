import * as midlevel from "@spx/sdk/midlevel";
import { IWorld } from "./interfaces.js";
import { ModuleMonitor } from "./module_monitor.js";
import { Logger } from "../log.js";

export class RCSController {
    private readonly monitor: ModuleMonitor<midlevel.CurrentThrust>;

    constructor(
        remote: midlevel.RCS,
        world: IWorld,
        logger: Logger,
        ship_id: string,
        slot_id: number,
        name: string,
    ) {
        this.monitor = new ModuleMonitor(remote, logger, name, (thrust) => {
            world.update({
                type: "rcs_update",
                ship_id,
                slot_id,
                thrust: {
                    thrust: thrust.thrust,
                    direction: { x: thrust.x, y: thrust.y },
                },
            });
        });
    }

    start(): void {
        this.monitor.start();
    }

    async stop(): Promise<void> {
        await this.monitor.stop();
    }
}

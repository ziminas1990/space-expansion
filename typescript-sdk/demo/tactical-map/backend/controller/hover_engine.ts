import * as midlevel from "@spx/sdk/midlevel";
import { IWorld } from "./interfaces.js";
import { ModuleMonitor } from "./module_monitor.js";
import { Logger } from "../log.js";

export class HoverEngineController {
    private readonly monitor: ModuleMonitor<number>;

    constructor(
        remote: midlevel.HoverEngine,
        world: IWorld,
        logger: Logger,
        ship_id: string,
        slot_id: number,
        name: string,
    ) {
        this.monitor = new ModuleMonitor(remote, logger, name, (thrust) => {
            world.update({ type: "hover_engine_update", ship_id, slot_id, thrust });
        });
    }

    start(): void {
        this.monitor.start();
    }

    async stop(): Promise<void> {
        await this.monitor.stop();
    }
}

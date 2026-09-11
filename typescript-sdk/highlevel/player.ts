import * as midlevel from "#sdk/midlevel/index.js";
import { Status } from "#sdk/types/status.js";
import { BlueprintsLibrary } from "./blueprints_library.js";
import { EventEmitter } from "./events.js";
import type { CreateModule } from "./factory.js";
import { Messanger } from "./messanger.js";
import { ModuleRegistry } from "./module_registry.js";
import type { HighlevelModule } from "./module_types.js";
import { ModuleType } from "./module_types.js";
import { Ship } from "./ship.js";
import { SystemClock } from "./system_clock.js";

export type Events = {
    ship_attached: (ship: Ship) => Promise<void> | void;
    ship_detached: (ship: Ship) => Promise<void> | void;
};

export class Player extends EventEmitter<Events> {

    private readonly registry: ModuleRegistry;

    constructor(
        private commutator: midlevel.Commutator,
        create_module: CreateModule,
    ) {
        super();
        this.registry = new ModuleRegistry(commutator, create_module);
        this.registry.on("attached", this.module_attached.bind(this));
        this.registry.on("detached", this.module_detached.bind(this));
    }

    system_clock(): SystemClock | undefined {
        return this.registry.get_all(ModuleType.SYSTEM_CLOCK)[0];
    }

    blueprints_library(): BlueprintsLibrary | undefined {
        return this.registry.get_all(ModuleType.BLUEPRINTS_LIBRARY)[0];
    }

    messanger(): Messanger | undefined {
        return this.registry.get_all(ModuleType.MESSANGER)[0];
    }

    get ships(): Ship[] {
        return this.registry.get_all(ModuleType.SHIP);
    }

    down_level(): midlevel.Commutator {
        return this.commutator;
    }

    async init(): Promise<Status> {
        const status = await this.registry.init();
        if (!status.is_ok()) {
            return status.wrap("Failed to init root commutator");
        }

        const clock = this.system_clock();
        if (clock) {
            const sync = await clock.initial_sync();
            if (!sync.is_ok()) {
                return sync.wrap("Failed to sync system clock");
            }
        }

        return Status.ok();
    }

    async release(): Promise<Status> {
        return await this.registry.release();
    }

    private async module_attached(module: HighlevelModule) {
        if (module.type === ModuleType.SHIP) {
            await this.emit("ship_attached", module);
        }
    }

    private async module_detached(module: HighlevelModule) {
        if (module.type === ModuleType.SHIP) {
            await this.emit("ship_detached", module);
        }
    }

}

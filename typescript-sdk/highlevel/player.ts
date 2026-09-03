import * as midlevel from "../midlevel/index.js";
import { Status } from "../types/status.js";
import { BlueprintsLibrary } from "./blueprints_library.js";
import { EventEmitter } from "./events.js";
import type { CreateModule } from "./factory.js";
import { Game } from "./game.js";
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

    public readonly game: Game;
    private readonly registry: ModuleRegistry;

    constructor(
        private commutator: midlevel.Commutator,
        private game_rpc: midlevel.Game,
        create_module: CreateModule,
    ) {
        super();
        this.game = new Game(game_rpc);
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

    down_level(what: "root_commutator"): midlevel.Commutator;
    down_level(what: "game"): midlevel.Game;
    down_level(what: "root_commutator" | "game"):
        midlevel.Commutator | midlevel.Game
    {
        switch (what) {
            case "root_commutator": return this.commutator;
            case "game": return this.game_rpc;
        }
    }

    async init(): Promise<Status> {
        const status = await this.registry.init();
        if (!status.is_ok()) {
            return status.wrap("Failed to init root commutator");
        }

        const game_status = await this.game.init();
        if (!game_status.is_ok()) {
            return game_status.wrap("Failed to init game");
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
        await this.game.release();
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

import * as midlevel from "#sdk/midlevel/index.js";
import { Position, Status } from "#sdk/types/index.js";
import { Cached } from "#sdk/utils/cache.js";
import type { CreateModule } from "./factory.js";
import { ModuleRegistry } from "./module_registry.js";
import { Navigation } from "./navigation.js";
import type { BaseModule } from "./base_module.js";
import { EventEmitter } from "./events.js";
import type { HighlevelModule } from "./module_types.js";

export type { Position };
export type ShipState = midlevel.ShipState;

export type Events = {
    attached: (module: HighlevelModule) => Promise<void> | void;
    detached: (module: HighlevelModule) => Promise<void> | void;
};

const DEFAULT_STATE_CACHE_MS = 50;

export class Ship extends EventEmitter<Events> implements BaseModule {
    readonly type = midlevel.ModuleType.SHIP;
    readonly ship_class: string;
    private navigation: Navigation;
    private registry: ModuleRegistry;

    private state = new Cached<ShipState>();

    private monitor: Promise<Status> | undefined = undefined;
    private stop_monitoring: boolean = false;

    constructor(
        private ship: midlevel.Ship,
        public name: string,
        ship_class: string,
        private readonly create_module: CreateModule,
    )
    {
        super();
        this.ship_class = ship_class;
        this.navigation = new Navigation(ship.navigator());
        this.registry = this.bind_registry(ship.commutator());
    }

    modules(): HighlevelModule[] {
        const result: HighlevelModule[] = [];
        for (const by_name of this.registry.attached.values()) {
            result.push(...by_name.values());
        }
        return result;
    }

    get_all<T extends midlevel.ModuleType>(
        type: T,
    ): Extract<HighlevelModule, { type: T }>[] {
        return this.registry.get_all(type);
    }

    get_by_name<T extends midlevel.ModuleType>(
        type: T,
        name: string,
    ): Extract<HighlevelModule, { type: T }> | undefined {
        return this.registry.get_by_name(type, name);
    }

    down_level(what: "ship"): midlevel.Ship;
    down_level(what: "navigator"): midlevel.Navigation;
    down_level(what: "commutator"): midlevel.Commutator;
    down_level(what: "ship" | "navigator" | "commutator"):
        midlevel.Ship | midlevel.Navigation | midlevel.Commutator
    {
        switch (what) {
            case "ship": return this.ship;
            case "navigator": return this.ship.navigator();
            case "commutator": return this.ship.commutator();
        }
    }

    async init(): Promise<Status> {
        this.stop_monitoring = false;
        const status = await this.registry.init();
        if (!status.is_ok()) {
            return status.wrap("failed to get ship modules");
        }
        const [state_status, state] = await this.get_state();
        if (!state_status.is_ok() || !state) {
            return state_status.wrap("failed to get ship state");
        }
        this.monitor = this.monitor_ship_state();
        return Status.ok();
    }

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.SHIP)) {
            return Status.fail("expected Ship");
        }
        await this.release();
        this.ship = rpc;
        this.navigation = new Navigation(rpc.navigator());
        this.registry = this.bind_registry(rpc.commutator());
        return await this.init();
    }

    async release(): Promise<Status> {
        this.stop_monitoring = true;
        await this.ship.terminate();
        if (this.monitor) {
            await this.monitor;
            this.monitor = undefined;
        }
        await this.registry.release();
        await this.navigation.release();
        this.state.reset();
        return Status.ok();
    }

    async get_state(cache_expiring_ms: number = DEFAULT_STATE_CACHE_MS)
        : Promise<[Status, ShipState | undefined]>
    {
        const cached = this.state.get(cache_expiring_ms);
        if (cached) {
            return [Status.ok(), cached];
        }
        const [status, state] = await this.ship.get_state();
        if (status.is_ok() && state) {
            this.cache_state(state);
        }
        return [status, state];
    }

    async get_position(at_us?: bigint, cache_expiring_ms: number = 10)
        : Promise<[Status, Position | undefined]>
    {
        return this.navigation.get_position(at_us, cache_expiring_ms);
    }

    predict_position(at_us: bigint): Position | undefined {
        return this.navigation.predict_position(at_us);
    }

    private bind_registry(commutator: midlevel.Commutator): ModuleRegistry {
        const registry = new ModuleRegistry(commutator, this.create_module);
        registry.on("attached", (module) => this.emit("attached", module));
        registry.on("detached", (module) => this.emit("detached", module));
        return registry;
    }

    private cache_state(state: ShipState) {
        this.state.set(state);
        if (state.position) {
            this.navigation.update_from(state.position);
        }
    }

    private handle_update(update: ShipState) {
        const current = this.state.get(Infinity);
        if (current == undefined) {
            this.cache_state(update);
            return;
        }

        if (update.position) {
            current.position = update.position;
            this.navigation.update_from(update.position);
        }
        if (update.weight) {
            current.weight = update.weight;
        }
        current.timestamp = update.timestamp;
        this.state.set(current);
    }

    private async monitor_ship_state(): Promise<Status> {
        while (!this.stop_monitoring) {
            const status = await this.ship.monitoring(100, async (state) => {
                if (state) {
                    this.handle_update(state);
                }
                return !this.stop_monitoring;
            });
            if (this.stop_monitoring || !status.is_ok()) {
                return Status.ok();
            }
        }
        return Status.ok();
    }

}

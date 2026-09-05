import {
    ModuleType,
    type Player,
    type ResourceContainerContent,
    type Ship as RemoteShip,
    type SystemClock,
} from "@spx/sdk/highlevel";
import { create_logger } from "./log.js";
import { Ship } from "./ship.js";
import { RandomMining } from "./tasks/mining/random_mining.js";
import {
    find_most_productive_shipyard,
    find_most_voluminous_container,
} from "./equipment.js";
import { is_abort_error, sleep, wait_abort } from "./util.js";
import { World } from "./world.js";

export class TacticalCore {
    readonly player: Player;
    readonly world = new World();
    readonly ships = new Map<string, Ship>();
    system_clock?: SystemClock;
    private readonly log = create_logger("TacticalCore");
    private readonly shutdown = new AbortController();

    private random_mining_task?: RandomMining;
    private build_miners_task?: Promise<void>;
    private time_monitoring_task?: Promise<void>;

    constructor(player: Player) {
        this.player = player;
    }

    private get stopping(): AbortSignal {
        return this.shutdown.signal;
    }

    async initialize(): Promise<boolean> {
        const clock = this.player.system_clock();
        if (!clock) {
            this.log.error("SystemClock not found!");
            return false;
        }
        this.system_clock = clock;

        this.time_monitoring_task = this.monitor_time();

        this.player.on("ship_attached", (remote) => this.on_ship_spawned(remote));

        for (const remote of this.player.ships) {
            await this.attach_ship(remote);
        }
        return true;
    }

    async stop(): Promise<void> {
        if (!this.stopping.aborted) {
            this.shutdown.abort();
        }
        this.random_mining_task?.interrupt();
        for (const ship of this.ships.values()) {
            ship.stop();
        }
        await this.player.release();
        const tasks = [
            this.build_miners_task,
            this.time_monitoring_task,
        ].filter((task): task is Promise<void> => task !== undefined);
        if (tasks.length > 0) {
            await Promise.allSettled(tasks);
        }
    }

    async run(): Promise<void> {
        const clock = this.system_clock;
        if (!clock) {
            this.log.error("SystemClock not found!");
            return;
        }

        const warehouses = this.get_ships_by_equipment([
            ModuleType.SHIPYARD,
            ModuleType.RESOURCE_CONTAINER,
        ]);
        const warehouse = warehouses[0];
        if (!warehouse) {
            this.log.error("Can't get Warehouse!");
            return;
        }
        this.log.info(`Using '${warehouse.name}' as warehouse`);

        this.random_mining_task = new RandomMining(
            "RandomMining", this, warehouse, clock,
        );
        const mining_ships = this.get_ships_by_equipment([
            ModuleType.RESOURCE_CONTAINER,
            ModuleType.ASTEROID_MINER,
            ModuleType.ENGINE,
        ]);
        for (const miner of mining_ships) {
            this.log.info(`Using '${miner.name}' as mining ship`);
            this.random_mining_task.add_ship(miner);
        }
        this.random_mining_task.run_async();

        this.build_miners_task = this.build_more_miners();
        await wait_abort(this.stopping);
    }

    get_ships_by_equipment(modules: ModuleType[]): Ship[] {
        const ships: Ship[] = [];
        for (const ship of this.ships.values()) {
            if (ship.has_modules(modules)) {
                ships.push(ship);
            }
        }
        return ships;
    }

    private async build_more_miners(): Promise<void> {
        const warehouses = this.get_ships_by_equipment([
            ModuleType.SHIPYARD,
            ModuleType.RESOURCE_CONTAINER,
        ]);
        const warehouse = warehouses[0];
        if (!warehouse) {
            this.log.error("Can't get Warehouse!");
            return;
        }

        const shipyard = await find_most_productive_shipyard(warehouse.remote);
        if (!shipyard) {
            this.log.error("Can't get shipyard!");
            return;
        }

        const cargo = await find_most_voluminous_container(warehouse.remote);
        if (!cargo) {
            this.log.error("Can't get resource container!");
            return;
        }

        cargo.on("content", (content: ResourceContainerContent) => {
            this.log.info(`Shipyard cargo: ${format_cargo(content)}`);
        });
        await shipyard.bind_to_cargo(cargo.name);

        let next_id = 10;
        const ship_type = "Ship/Civilian-Miner";
        let backoff_ms = 500;
        while (!this.stopping.aborted) {
            const [status] = await shipyard.build_ship(
                ship_type,
                `Miner-${next_id}`,
                (build_status, progress) => {
                    this.log.info(`Shipyard: ${build_status} ${progress}`);
                },
            );
            if (status.is_ok()) {
                next_id += 1;
                backoff_ms = 500;
                continue;
            }
            this.log.warning(`Failed to build miner: ${status.what()}`);
            try {
                await sleep(backoff_ms, this.stopping);
            } catch (error) {
                if (is_abort_error(error)) {
                    return;
                }
                throw error;
            }
            backoff_ms = Math.min(backoff_ms * 2, 32_000);
        }
    }

    private on_ship_spawned(remote: RemoteShip): void {
        const ship = this.attach_ship_sync(remote);
        if (!ship) {
            return;
        }
        void ship.start_passive_scanning();
        if (this.random_mining_task && RandomMining.can_use_ship(ship)) {
            this.random_mining_task.add_ship(ship);
        }
    }

    private async attach_ship(remote: RemoteShip): Promise<Ship | undefined> {
        const ship = this.attach_ship_sync(remote);
        if (!ship) {
            return undefined;
        }
        await ship.start_passive_scanning();
        return ship;
    }

    private attach_ship_sync(remote: RemoteShip): Ship | undefined {
        const clock = this.system_clock;
        if (!clock) {
            return undefined;
        }
        const existing = this.ships.get(remote.name);
        if (existing) {
            return existing;
        }
        const ship = new Ship(remote, this.world, clock);
        this.ships.set(ship.name, ship);
        return ship;
    }

    private async monitor_time(): Promise<void> {
        const clock = this.system_clock;
        if (!clock) {
            return;
        }
        while (!this.stopping.aborted) {
            await clock.monitoring(40, () => !this.stopping.aborted);
            if (this.stopping.aborted) {
                return;
            }
            await sleep(250, this.stopping).catch((error) => {
                if (!is_abort_error(error)) {
                    throw error;
                }
            });
        }
    }
}

function format_cargo(content: ResourceContainerContent): string {
    const items = content.resources
        .map((item) => `${item.resource_type}: ${item.amount}`)
        .join(", ");
    return `volume=${content.volume} used=${content.used} [${items}]`;
}

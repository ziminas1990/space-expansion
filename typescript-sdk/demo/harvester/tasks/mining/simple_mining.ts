import type {
    AsteroidMiner,
    ResourceContainer,
    ResourceContainerContent,
    SystemClock,
} from "../../../../highlevel/index.js";
import type { ResourceItem } from "../../../../types/index.js";
import { find_most_efficient_miner, find_most_free_container } from "../../equipment.js";
import type { Ship } from "../../ship.js";
import type { TacticalCore } from "../../tactical_core.js";
import { random_int } from "../../util.js";
import { BaseTask } from "../base_task.js";

function format_content(content: ResourceContainerContent): string {
    const used_pct = content.volume > 0 ? (100 * content.used / content.volume) : 0;
    const items = content.resources
        .map((item) => `${item.resource_type}: ${item.amount}`)
        .join(", ");
    return `${used_pct.toFixed(2)}% used: ${items}`;
}

export class SimpleMining extends BaseTask {
    private readonly tactical_core: TacticalCore;
    private readonly ship: Ship;
    private readonly asteroid_id: number;
    private readonly warehouse: Ship;

    private _miner?: AsteroidMiner;
    private _container?: ResourceContainer;

    constructor(
        name: string,
        tactical_core: TacticalCore,
        ship: Ship,
        asteroid_id: number,
        warehouse: Ship,
        system_clock: SystemClock,
    ) {
        super(name, system_clock);
        this.tactical_core = tactical_core;
        this.ship = ship;
        this.asteroid_id = asteroid_id;
        this.warehouse = warehouse;
    }

    override interrupt(): void {
        this.ship.navigator.interrupt();
        super.interrupt();
    }

    protected async _impl(): Promise<boolean> {
        if (!await this._initial_checks()) {
            this.add_journal_record("Initial checks failed");
            return false;
        }
        this.add_journal_record("Initial checks done");

        if (!await this._move_to_asteroid()) {
            this.add_journal_record("Moving to asteroid failed!");
            return false;
        }
        this.add_journal_record("Approached to the asteroid");

        if (!await this._do_mining()) {
            this.add_journal_record("Mining failed!");
            return false;
        }
        this.add_journal_record("Mining finished");

        if (!await this._return_to_warehouse()) {
            this.add_journal_record("Failed to return to warehouse");
            return false;
        }
        this.add_journal_record("Returned to the warehouse");

        if (!await this._unload_the_product()) {
            this.add_journal_record("Failed to unload ore");
            return false;
        }
        this.add_journal_record("Ore is unloaded");

        this.add_journal_record("Finished successful!");
        return true;
    }

    private async _initial_checks(): Promise<boolean> {
        this._miner = await find_most_efficient_miner(this.ship.remote);
        if (!this._miner) {
            this.add_journal_record("Can't find appropriate miner!");
            return false;
        }
        this.add_journal_record(`Use '${this._miner.name}' as miner`);

        this._container = await find_most_free_container(this.ship.remote);
        if (!this._container) {
            this.add_journal_record("Can't find appropriate container!");
            return false;
        }
        this.add_journal_record(`Use '${this._container.name}' as container`);
        return true;
    }

    private async _move_to_asteroid(): Promise<boolean> {
        const asteroid = this.tactical_core.world.asteroids.get(this.asteroid_id);
        if (!asteroid) {
            return false;
        }
        return await this.ship.navigator.move_to({
            ...asteroid.position,
            timestamp: this.system_clock.now_us(),
        });
    }

    private async _do_mining(): Promise<boolean> {
        const miner = this._miner;
        const container = this._container;
        if (!miner || !container) {
            return false;
        }

        const bind_status = await miner.bind_to_cargo(container.name);
        if (!bind_status.is_ok()) {
            this.add_journal_record(
                `Failed to attach '${miner.name}' miner to `
                + `'${container.name}' container: ${bind_status.what()}`,
            );
            return false;
        }

        const on_report = () => {
            void container.get_content(0).then(([status, content]) => {
                if (status.is_ok() && content) {
                    this.add_journal_record(format_content(content));
                }
            });
        };
        miner.on("mining_report", on_report);
        try {
            const status = await miner.start_mining(this.asteroid_id);
            this.add_journal_record(`Mining complete with status ${status.what()}`);
            return status.is_ok() || status.what().includes("NO_SPACE_AVAILABLE");
        } finally {
            miner.off("mining_report", on_report);
        }
    }

    private async _return_to_warehouse(): Promise<boolean> {
        const [status, position] = await this.warehouse.remote.get_position();
        if (!status.is_ok() || !position) {
            return false;
        }
        return await this.ship.navigator.move_to(position);
    }

    private async _unload_the_product(): Promise<boolean> {
        const container = this._container;
        if (!container) {
            return false;
        }

        const target_container = await find_most_free_container(this.warehouse.remote);
        if (!target_container) {
            this.add_journal_record(
                `Can't get resource container at '${this.warehouse.name}' warehouse`,
            );
            return false;
        }
        this.add_journal_record(
            `Unloading resources to '${target_container.name}' container`,
        );

        if (target_container.opened_port === undefined) {
            const access_key = random_int(2 ** 16);
            const [status] = await target_container.open_port(access_key);
            if (!status.is_ok()) {
                this.add_journal_record(`Failed to open port: ${status.what()}`);
                return false;
            }
        }

        const opened = target_container.opened_port;
        if (opened === undefined) {
            this.add_journal_record("Failed to open port: port is missing");
            return false;
        }
        const [port, access_key] = opened;

        const [content_status, content] = await container.get_content();
        if (!content_status.is_ok() || content === undefined) {
            this.add_journal_record("Failed to get content");
            return false;
        }

        const transfer_status = (resource: ResourceItem) => {
            this.add_journal_record(
                `${resource.resource_type}: ${resource.amount} transferred`,
            );
        };

        for (const resource of content.resources) {
            const status = await container.transfer(
                port,
                access_key,
                resource,
                transfer_status,
            );
            if (!status.is_ok()) {
                this.add_journal_record(
                    `Can't transfer ${resource.resource_type}: ${resource.amount} `
                    + `to warehouse: ${status.what()}`,
                );
            }
        }
        return true;
    }
}

import * as midlevel from "@spx/sdk/midlevel";
import { Status } from "@spx/sdk/types";
import { PlayerShip } from "../../common/domain/player_ship.js";
import { EntityRef } from "../../common/domain/world.js";
import { convert_orientation, convert_position } from "./helpers.js";
import { IWorld } from "./interfaces.js";
import { Logger } from "../log.js";
import { PassiveScanner } from "./passive_scanner.js";
import { RetryTimeout } from "../utils/retry_timeout.js";

const STATE_MONITOR_MS = 100;
const MONITOR_RETRY_MS = [500, 1000, 2000, 5000];

export class Ship {

    private stopped: boolean = false;
    private stop_task: Promise<Status> | undefined;
    private modules_monitoring_task?: Promise<void>;
    private state_monitoring_task?: Promise<void>;

    private readonly modules: Map<number, midlevel.ModuleInfo> = new Map();
    private readonly passive_scanners: Map<number, PassiveScanner> = new Map();

    constructor(
        private readonly remote: midlevel.Ship,
        private readonly world: IWorld,
        private readonly logger: Logger,
        private readonly name: string,
    ) {}

    async initialize(): Promise<Status> {
        const [state_status, state] = await this.remote.get_state();
        if (!state_status.is_ok() || !state) {
            await this.remote.terminate();
            return state_status.wrap("Failed to get ship state");
        }
        this.apply_state(state);

        const [status, modules] =
            await this.remote.commutator().get_all_modules_info();
        if (!status.is_ok()) {
            this.remove_from_world();
            await this.remote.terminate();
            return status.wrap("Failed to get ship modules");
        }

        for (const module of modules!) {
            const module_status = await this.on_module_attached(module);
            if (!module_status.is_ok()) {
                this.logger.error(
                    `Failed to attach module '${module.module_name}' `
                    + `to ship '${this.name}': ${module_status.what()}`,
                );
            }
        }

        this.state_monitoring_task = this.monitor_state();
        this.modules_monitoring_task = this.monitor_modules();
        return Status.ok();
    }

    async stop(): Promise<Status> {
        if (this.stop_task) {
            return this.stop_task;
        }
        this.stopped = true;
        this.stop_task = this.run_stop();
        return this.stop_task;
    }

    private async run_stop(): Promise<Status> {
        this.remove_from_world();
        const scanners = [...this.passive_scanners.values()];
        this.passive_scanners.clear();
        this.modules.clear();
        await Promise.all(scanners.map((scanner) => scanner.stop()));
        await this.remote.terminate();
        if (this.state_monitoring_task) {
            await this.state_monitoring_task;
            this.state_monitoring_task = undefined;
        }
        if (this.modules_monitoring_task) {
            await this.modules_monitoring_task;
            this.modules_monitoring_task = undefined;
        }
        return Status.ok();
    }

    private entity_ref(): EntityRef {
        return { kind: "player_ship", id: this.name };
    }

    private remove_from_world(): void {
        this.world.update({
            type: "remove_entity",
            entity: this.entity_ref(),
        });
    }

    private apply_state(state: midlevel.ShipState): void {
        if (this.stopped) {
            return;
        }
        const position = convert_position(state.position);
        const orientation = convert_orientation(state.orientation);
        if (this.world.has_entity(this.entity_ref())) {
            this.world.update({
                type: "player_ship_update",
                ship_id: this.name,
                update: { position, orientation },
            });
        } else {
            this.world.update({
                type: "add_player_ship",
                ship: new PlayerShip(this.name, position, orientation),
            });
        }
    }

    private async monitor_state() {
        const retry_timeout = new RetryTimeout(MONITOR_RETRY_MS);
        while (!this.stopped) {
            const status = await this.remote.monitoring(
                STATE_MONITOR_MS,
                this.handle_state.bind(this),
            );
            if (!this.stopped && !status.is_ok()) {
                this.logger.error(
                    `Ship '${this.name}' state monitoring failed: ${status.what()}`,
                );
                await retry_timeout.wait_to_retry(() => this.stopped);
            } else {
                retry_timeout.reset();
            }
        }
    }

    private async handle_state(
        state: midlevel.ShipState | undefined,
    ): Promise<boolean>
    {
        if (this.stopped) {
            return false;
        }
        if (state !== undefined) {
            this.apply_state(state);
        }
        return true;
    }

    private async monitor_modules() {
        const retry_timeout = new RetryTimeout(MONITOR_RETRY_MS);
        while (!this.stopped) {
            const status = await this.remote.commutator().monitoring(
                this.handle_module_update.bind(this),
            );
            if (!this.stopped && !status.is_ok()) {
                this.logger.error(
                    `Ship '${this.name}' modules monitoring failed: ${status.what()}`,
                );
                await retry_timeout.wait_to_retry(() => this.stopped);
            } else {
                retry_timeout.reset();
            }
        }
    }

    private async handle_module_update(
        update: midlevel.CommutatorUpdate | undefined,
    ): Promise<boolean>
    {
        if (update?.module_attached) {
            const status = await this.on_module_attached(update.module_attached);
            if (!status.is_ok()) {
                this.logger.error(
                    `Failed to attach module '${update.module_attached.module_name}' `
                    + `to ship '${this.name}': ${status.what()}`,
                );
            }
        }
        if (update?.module_detached !== undefined) {
            await this.on_module_detached(update.module_detached);
        }
        return !this.stopped;
    }

    private async on_module_attached(info: midlevel.ModuleInfo): Promise<Status> {
        const current = this.modules.get(info.slot_id);
        if (current) {
            await this.on_module_detached(info.slot_id);
        }
        this.modules.set(info.slot_id, info);

        if (info.module_type !== midlevel.ModuleType.PASSIVE_SCANNER) {
            return Status.ok();
        }

        const scanner = new PassiveScanner(
            new midlevel.PassiveScanner(info.open_session_cb),
            this.world,
            this.logger.child(info.module_name),
            info.module_name,
        );
        const status = await scanner.initialize();
        if (!status.is_ok()) {
            this.modules.delete(info.slot_id);
            return status.wrap("Failed to initialize passive scanner");
        }
        this.passive_scanners.set(info.slot_id, scanner);
        return Status.ok();
    }

    private async on_module_detached(slot_id: number): Promise<void> {
        const info = this.modules.get(slot_id);
        if (!info) {
            return;
        }
        this.modules.delete(slot_id);

        if (info.module_type === midlevel.ModuleType.PASSIVE_SCANNER) {
            const scanner = this.passive_scanners.get(slot_id);
            if (scanner) {
                await scanner.stop();
                this.passive_scanners.delete(slot_id);
            }
        }
    }
}
